/** @file WSService.cpp
 *  @brief Implementation of UWSService. All logic documentation is in the header.
 */
#include "Services/WSService.h"
#include "Misc/Base64.h"
#include "WebSocketsModule.h"
#include "IWebSocket.h"
#include "TimerManager.h"
#include "Engine/World.h"

void UWSService::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    const FString SecretsFile = FPaths::ProjectConfigDir() / TEXT("Secrets.ini");
    GConfig->LoadFile(SecretsFile);
    FString Host, Username, Password, WsStartMessage;
    GConfig->GetString(TEXT("Ditto"), TEXT("Host"),           Host,           SecretsFile);
    GConfig->GetString(TEXT("Ditto"), TEXT("Username"),       Username,       SecretsFile);
    GConfig->GetString(TEXT("Ditto"), TEXT("Password"),       Password,       SecretsFile);
    GConfig->GetString(TEXT("Ditto"), TEXT("WsStartMessage"), WsStartMessage, SecretsFile);

    if (Host.IsEmpty() || Username.IsEmpty() || Password.IsEmpty() || WsStartMessage.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("WSService: one or more values missing from Config/Secrets.ini"));
    }

    const FString WsUrl        = TEXT("ws://") + Host + TEXT("/ws/2");
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
        if (Socket->IsConnected())
        {
            Socket->Close(1000, TEXT("Shutdown"));
        }

        Socket.Reset();
    }

    UE_LOG(LogTemp, Log, TEXT("WSService properly shut down"));
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
        UE_LOG(LogTemp, Log, TEXT("WebSocket connecting to %s with Authorization header"), *Url);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("WebSocket connecting to %s WITHOUT Authorization header"), *Url);
    }

    for (auto &Pair : Headers)
    {
        UE_LOG(LogTemp, Warning, TEXT("Header: %s = %s"), *Pair.Key, *Pair.Value);
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
			UE_LOG(LogTemp, Log, TEXT("WebSocket connected"));
			CurrentRetryCount = 0;  // Reset retry count on successful connection
			ClearReconnectTicker();
			OnConnected.Broadcast();

			if (!StartMessage.IsEmpty())
			{
				Socket->Send(StartMessage);
			} });

    Socket->OnMessage().AddLambda([this](const FString &Message)
                                  { OnMessage.Broadcast(Message); });

    Socket->OnClosed().AddLambda([this](int32 StatusCode, const FString &Reason, bool bWasClean)
                                 {
			if (!bIsDestroying)
			{
				UE_LOG(LogTemp, Warning, TEXT("WebSocket closed (%d): %s"), StatusCode, *Reason);
				OnClosed.Broadcast(StatusCode, Reason);

				if (!bManualClose && bAutoReconnect)
				{
					ScheduleReconnect();
				}
			} });

    Socket->OnConnectionError().AddLambda([this](const FString &Error)
                                          {
			if (bIsDestroying)
			{
				return;  // Ignore errors during shutdown
			}

			if (CurrentRetryCount < MaxRetryAttempts)
			{
				UE_LOG(LogTemp, Error, TEXT("WebSocket error: %s (Retry %d/%d)"), *Error, CurrentRetryCount + 1, MaxRetryAttempts);
				OnError.Broadcast(Error);

				if (!bManualClose && bAutoReconnect)
				{
					ScheduleReconnect();
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("WebSocket error: %s (Max retries reached, giving up)"), *Error);
				OnError.Broadcast(Error);
			} });
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
