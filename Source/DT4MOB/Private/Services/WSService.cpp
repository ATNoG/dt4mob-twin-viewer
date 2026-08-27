/** @file WSService.cpp
 *  @brief Implementation of UWSService. All logic documentation is in the header.
 */
#include "Services/WSService.h"
#include "Services/DittoService.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
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

    // Ensure DittoService is initialized first — it owns the connection config (Host,
    // UseHttps, WsStartMessage) that this service also needs, so we read it from there
    // instead of loading the secrets asset a second time.
    Collection.InitializeDependency<UDittoService>();
    UDittoService* Ditto = GetGameInstance()->GetSubsystem<UDittoService>();
    check(Ditto);

    // Host/WsStartMessage aren't known yet at this point — DittoService no longer
    // auto-authenticates at Initialize() time, it waits for Login() to be called from the
    // login screen. Url/StartMessage are built lazily in HandleAuthHeaderReady() instead, once
    // login has actually happened and Ditto->GetHost() is populated.
    bAutoReconnect = true;
    ReconnectDelaySeconds = 5.0f;

    Ditto->OnAuthHeaderReady.AddDynamic(this, &UWSService::HandleAuthHeaderReady);
    Ditto->OnLoggedOut.AddDynamic(this, &UWSService::HandleDittoLoggedOut);

    // Basic auth fires OnAuthHeaderReady synchronously from within Login(), which means WSService
    // could in principle miss it if Login() were somehow already complete before this binds —
    // check if auth is already available and connect now, just in case.
    const FString CurrentHeader = Ditto->GetCurrentAuthHeader();
    if (!CurrentHeader.IsEmpty())
    {
        HandleAuthHeaderReady(CurrentHeader);
    }
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

        // Re-apply geotile filter on reconnect; fall back to StartMessage if none set.
        if (!ActiveFilter.IsEmpty())
        {
            const FString Cmd = TEXT("START-SEND-EVENTS?filter=") + ActiveFilter;
            Socket->Send(Cmd);
            UE_LOG(LogWSService, Log, TEXT("WSService: subscribed with filter: %s"), *Cmd);
        }
        else if (!StartMessage.IsEmpty())
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

void UWSService::SetEventFilter(const FString& Filter)
{
    if (ActiveFilter == Filter)
        return;

    ActiveFilter = Filter;

    if (!IsConnected())
        return;

    Socket->Send(TEXT("STOP-SEND-EVENTS"));

    if (Filter.IsEmpty())
    {
        Socket->Send(StartMessage);
        UE_LOG(LogWSService, Log, TEXT("WSService: filter cleared, resubscribed to all events"));
    }
    else
    {
        const FString Cmd = TEXT("START-SEND-EVENTS?filter=") + Filter;
        Socket->Send(Cmd);
        UE_LOG(LogWSService, Log, TEXT("WSService: updated event filter: %s"), *Cmd);
    }
}

void UWSService::HandleAuthHeaderReady(const FString& NewAuthHeader)
{
    AuthHeader = NewAuthHeader;

    // Rebuild Url/StartMessage from DittoService every time — Host is only actually known once
    // Login() has run, and this may be the first time auth has ever been ready this session.
    if (UDittoService* Ditto = GetGameInstance() ? GetGameInstance()->GetSubsystem<UDittoService>() : nullptr)
    {
        const FString Host = Ditto->GetHost();
        if (Host.IsEmpty())
        {
            UE_LOG(LogWSService, Warning, TEXT("WSService: auth ready but Host is empty, cannot connect"));
            return;
        }

        Url = (Ditto->IsUseHttps() ? TEXT("wss://") : TEXT("ws://")) + Host + TEXT("/ws/2");
        StartMessage = Ditto->GetWsStartMessage();
    }

    // A fresh login (possibly after a prior Logout()) should always be allowed to (re)connect.
    bManualClose = false;

    // Only open a new connection if we're not already connected or mid-reconnect.
    if (!IsConnected() && !bIsDestroying)
    {
        CurrentRetryCount = 0;
        ConnectInternal();
        UE_LOG(LogWSService, Log, TEXT("WSService: auth ready, connecting (%s)"),
               NewAuthHeader.StartsWith(TEXT("Bearer")) ? TEXT("OAuth Bearer") : TEXT("Basic"));
    }
    else
    {
        UE_LOG(LogWSService, Log, TEXT("WSService: auth header updated for next reconnect"));
    }
}

void UWSService::HandleDittoLoggedOut()
{
    UE_LOG(LogWSService, Log, TEXT("WSService: logged out, disconnecting"));
    Disconnect();
}
