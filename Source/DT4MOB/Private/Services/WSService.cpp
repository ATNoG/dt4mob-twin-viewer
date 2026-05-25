/** @file WSService.cpp
 *  @brief Implementation of UWSService. All logic documentation is in the header.
 */
#include "Services/WSService.h"
#include "Misc/Base64.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY(LogWSService);

namespace
{

class FWSFileLogger : public FOutputDevice
{
public:
    FWSFileLogger()
    {
        const FString FilePath = FPaths::ProjectLogDir() / TEXT("WSService.log");
        FileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*FilePath, /*bAppend=*/true, /*bAllowRead=*/false);
    }

    virtual ~FWSFileLogger()
    {
        delete FileHandle;
    }

    virtual void Serialize(const TCHAR* V, ELogVerbosity::Type Verbosity, const FName& Category) override
    {
        if (Category != FName("LogWSService") || !FileHandle)
            return;

        const FString Line = FString(V) + LINE_TERMINATOR;
        const FTCHARToUTF8 Converted(*Line);
        FileHandle->Write(reinterpret_cast<const uint8*>(Converted.Get()), Converted.Length());
    }

    virtual bool CanBeUsedOnMultipleThreads() const override { return false; }

private:
    IFileHandle* FileHandle = nullptr;
};

} // namespace

void UWSService::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    WSFileLogger = new FWSFileLogger();
    GLog->AddOutputDevice(WSFileLogger);

    const FString SecretsFile = FPaths::ProjectConfigDir() / TEXT("Secrets.ini");
    GConfig->LoadFile(SecretsFile);
    FString Host, Username, Password, WsStartMessage;
    bool bUseHttps = true;
    GConfig->GetString(TEXT("Ditto"), TEXT("Host"),           Host,           SecretsFile);
    GConfig->GetString(TEXT("Ditto"), TEXT("Username"),       Username,       SecretsFile);
    GConfig->GetString(TEXT("Ditto"), TEXT("Password"),       Password,       SecretsFile);
    GConfig->GetString(TEXT("Ditto"), TEXT("WsStartMessage"), WsStartMessage, SecretsFile);
    GConfig->GetBool  (TEXT("Ditto"), TEXT("UseHttps"),       bUseHttps,      SecretsFile);

    if (Host.IsEmpty() || Username.IsEmpty() || Password.IsEmpty() || WsStartMessage.IsEmpty())
    {
        UE_LOG(LogWSService, Warning, TEXT("WSService: one or more values missing from Config/Secrets.ini"));
    }

    const FString WsUrl        = (bUseHttps ? TEXT("wss://") : TEXT("ws://")) + Host + TEXT("/ws/2");
    const FString WsAuthHeader = TEXT("Basic ") + FBase64::Encode(Username + TEXT(":") + Password);
    Connect(WsUrl, WsAuthHeader, WsStartMessage, true, 5.0f);
}

void UWSService::Deinitialize()
{
    bIsDestroying = true;
    bManualClose = true;

    ClearReconnectTicker();

    if (Socket.IsValid())
    {
        Socket->OnConnected().Clear();
        Socket->OnMessage().Clear();
        Socket->OnClosed().Clear();
        Socket->OnConnectionError().Clear();

        if (Socket->IsConnected())
            Socket->Close(1000, TEXT("Shutdown"));

        Socket.Reset();
    }

    UE_LOG(LogWSService, Log, TEXT("WSService: shut down"));

    if (WSFileLogger)
    {
        GLog->RemoveOutputDevice(WSFileLogger);
        delete WSFileLogger;
        WSFileLogger = nullptr;
    }
}

void UWSService::Connect(const FString &InUrl, const FString &InAuthHeader, const FString &InStartMessage, bool bInAutoReconnect, float InReconnectDelaySeconds)
{
    Url = InUrl;
    AuthHeader = InAuthHeader;
    StartMessage = InStartMessage;
    bAutoReconnect = bInAutoReconnect;
    ReconnectDelaySeconds = InReconnectDelaySeconds;
    bManualClose = false;
    CurrentRetryCount = 0; // Reset retry count on new connection attempt

    ConnectInternal();
}

void UWSService::Disconnect()
{
    bManualClose = true;
    ClearReconnectTicker();

    if (Socket.IsValid())
    {
        // Close the connection gracefully
        if (Socket->IsConnected())
        {
            Socket->Close(1000, TEXT("Normal closure"));
        }
        Socket.Reset();
    }
}

void UWSService::SendText(const FString &Message)
{
    if (Socket.IsValid() && Socket->IsConnected())
    {
        Socket->Send(Message);
    }
}

bool UWSService::IsConnected() const
{
    return Socket.IsValid() && Socket->IsConnected();
}

void UWSService::ConnectInternal()
{
    if (Socket.IsValid())
    {
        Socket->OnConnected().Clear();
        Socket->OnMessage().Clear();
        Socket->OnClosed().Clear();
        Socket->OnConnectionError().Clear();

        Socket->Close();
        Socket.Reset();
    }

    TMap<FString, FString> Headers;
    if (!AuthHeader.IsEmpty())
    {
        Headers.Add(TEXT("Authorization"), AuthHeader);
        UE_LOG(LogWSService, Log, TEXT("WSService: connecting to %s with Authorization header"), *Url);
    }
    else
    {
        UE_LOG(LogWSService, Warning, TEXT("WSService: connecting to %s WITHOUT Authorization header"), *Url);
    }

    Socket = FWebSocketsModule::Get().CreateWebSocket(Url, TEXT("ditto-protocol"), Headers);
    BindSocketHandlers();
    Socket->Connect();
}

void UWSService::BindSocketHandlers()
{
    if (!Socket.IsValid())
    {
        return;
    }

    Socket->OnConnected().AddLambda([this]()
    {
        UE_LOG(LogWSService, Log, TEXT("WSService: connected to %s"), *Url);
        CurrentRetryCount = 0;
        ClearReconnectTicker();
        OnConnected.Broadcast();

        if (!StartMessage.IsEmpty())
        {
            Socket->Send(StartMessage);
            UE_LOG(LogWSService, Log, TEXT("WSService: sent start message: %s"), *StartMessage);
        }
    });

    Socket->OnMessage().AddLambda([this](const FString &Message)
    {
        UE_LOG(LogWSService, Log, TEXT("WSService: received (%d chars): %s"), Message.Len(), *Message);
        OnMessage.Broadcast(Message);
    });

    Socket->OnClosed().AddLambda([this](int32 StatusCode, const FString &Reason, bool bWasClean)
    {
        if (!bIsDestroying)
        {
            UE_LOG(LogWSService, Warning, TEXT("WSService: closed (%d): %s"), StatusCode, *Reason);
            OnClosed.Broadcast(StatusCode, Reason);

            if (!bManualClose && bAutoReconnect)
            {
                ScheduleReconnect();
            }
        }
    });

    Socket->OnConnectionError().AddLambda([this](const FString &Error)
    {
        if (bIsDestroying)
            return;

        if (CurrentRetryCount < MaxRetryAttempts)
        {
            UE_LOG(LogWSService, Error, TEXT("WSService: error: %s (retry %d/%d)"), *Error, CurrentRetryCount + 1, MaxRetryAttempts);
            OnError.Broadcast(Error);

            if (!bManualClose && bAutoReconnect)
            {
                ScheduleReconnect();
            }
        }
        else
        {
            UE_LOG(LogWSService, Error, TEXT("WSService: error: %s (max retries reached, giving up)"), *Error);
            OnError.Broadcast(Error);
        }
    });
}

void UWSService::ScheduleReconnect()
{
    if (ReconnectDelaySeconds <= 0.0f || CurrentRetryCount >= MaxRetryAttempts)
    {
        return;
    }

    UWorld *World = GetWorld();
    if (!World)
    {
        return;
    }

    // Exponential backoff: delay = base_delay * (2 ^ retry_count)
    float BackoffDelay = ReconnectDelaySeconds * FMath::Pow(2.0f, FMath::Min(CurrentRetryCount, 4));
    CurrentRetryCount++;

    ClearReconnectTicker();
    World->GetTimerManager().SetTimer(
        ReconnectTimerHandle,
        this,
        &UWSService::HandleReconnectTick,
        BackoffDelay,
        false);
}

void UWSService::ClearReconnectTicker()
{
    UWorld *World = GetWorld();
    if (World && ReconnectTimerHandle.IsValid())
    {
        World->GetTimerManager().ClearTimer(ReconnectTimerHandle);
    }
}

void UWSService::HandleReconnectTick()
{
    if (!bIsDestroying)
    {
        ConnectInternal();
    }
}
