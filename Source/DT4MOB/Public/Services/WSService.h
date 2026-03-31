#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TimerManager.h"
#include "WSService.generated.h"

class IWebSocket;

/** @brief Broadcast when the WebSocket connection is successfully established. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FWSServiceConnected);

/** @brief Broadcast for every incoming WebSocket text message. @param Message Raw text message received from the server. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWSServiceMessage, const FString &, Message);

/** @brief Broadcast when a WebSocket connection error occurs. @param Error Human-readable error description. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWSServiceError, const FString &, Error);

/** @brief Broadcast when the WebSocket connection closes. @param StatusCode WebSocket close code. @param Reason Human-readable close reason. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWSServiceClosed, int32, StatusCode, const FString &, Reason);

/**
 * @brief GameInstance subsystem that manages a single persistent WebSocket connection.
 *
 * Supports automatic reconnection with exponential back-off (up to MaxRetryAttempts times)
 * when the connection drops unexpectedly. Manual disconnection (via Disconnect()) suppresses
 * the auto-reconnect logic.
 *
 * After a successful connection the service sends StartMessage to the server, which is used
 * by the Ditto WebSocket protocol to begin streaming twin events.
 *
 * Consumers should listen to OnConnected, OnMessage, OnError, and OnClosed rather than
 * interacting with the underlying IWebSocket directly.
 */
UCLASS()
class DT4MOB_API UWSService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /**
     * @brief Called by the engine when the subsystem is created. No socket is opened here.
     * @param Collection Subsystem collection.
     */
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;

    /**
     * @brief Closes the socket gracefully and cancels any pending reconnect timer.
     *
     * Sets bIsDestroying to suppress error callbacks during teardown.
     */
    virtual void Deinitialize() override;

    /**
     * @brief Opens a WebSocket connection to the specified URL.
     *
     * Resets the retry counter. If a socket already exists it is closed and recreated.
     *
     * @param InUrl                  WebSocket endpoint URL (e.g. "ws://host/ws/2").
     * @param InAuthHeader           Value for the Authorization header (e.g. "Basic <base64>").
     * @param InStartMessage         Message to send immediately after connection is established.
     * @param bInAutoReconnect       Whether to attempt reconnection on unexpected disconnection.
     * @param InReconnectDelaySeconds Base delay in seconds between reconnection attempts.
     */
    UFUNCTION(BlueprintCallable, Category = "WSService")
    void Connect(const FString &InUrl, const FString &InAuthHeader, const FString &InStartMessage, bool bInAutoReconnect = true, float InReconnectDelaySeconds = 5.0f);

    /**
     * @brief Closes the WebSocket connection and suppresses any subsequent auto-reconnect.
     */
    UFUNCTION(BlueprintCallable, Category = "WSService")
    void Disconnect();

    /**
     * @brief Sends a UTF-8 text frame if the socket is currently connected.
     * @param Message Text payload to send.
     */
    UFUNCTION(BlueprintCallable, Category = "WSService")
    void SendText(const FString &Message);

    /**
     * @brief Returns whether the socket is currently in the connected state.
     * @return True if the underlying IWebSocket exists and reports IsConnected().
     */
    UFUNCTION(BlueprintCallable, Category = "WSService")
    bool IsConnected() const;

    /** @brief Broadcast on successful connection establishment. */
    UPROPERTY(BlueprintAssignable, Category = "WSService")
    FWSServiceConnected OnConnected;

    /** @brief Broadcast for every incoming text message. */
    UPROPERTY(BlueprintAssignable, Category = "WSService")
    FWSServiceMessage OnMessage;

    /** @brief Broadcast on connection error (may be followed by a reconnect attempt). */
    UPROPERTY(BlueprintAssignable, Category = "WSService")
    FWSServiceError OnError;

    /** @brief Broadcast when the connection closes (cleanly or otherwise). */
    UPROPERTY(BlueprintAssignable, Category = "WSService")
    FWSServiceClosed OnClosed;

private:
    /** @brief The underlying WebSocket interface instance. */
    TSharedPtr<IWebSocket> Socket;

    /** @brief Stored URL, reused on reconnection attempts. */
    FString Url;

    /** @brief Stored Authorization header value, reused on reconnection. */
    FString AuthHeader;

    /** @brief Message sent to the server immediately after connecting (Ditto START-SEND-EVENTS command). */
    FString StartMessage;

    /** @brief Whether to automatically reconnect on unexpected close or error. */
    bool bAutoReconnect = true;

    /** @brief True when Disconnect() was called explicitly; suppresses auto-reconnect. */
    bool bManualClose = false;

    /** @brief True during Deinitialize() to suppress callbacks from in-flight close/error events. */
    bool bIsDestroying = false;

    /** @brief Base delay between reconnect attempts; actual delay uses exponential back-off. */
    float ReconnectDelaySeconds = 5.0f;

    /** @brief Timer handle for the scheduled reconnect attempt. */
    FTimerHandle ReconnectTimerHandle;

    /** @brief Current number of reconnection attempts since the last successful connection. */
    int32 CurrentRetryCount = 0;

    /** @brief Maximum number of consecutive reconnection attempts before giving up. */
    int32 MaxRetryAttempts = 5;

    /** @brief Creates the IWebSocket, adds the Authorization header, binds handlers, and calls Connect(). */
    void ConnectInternal();

    /** @brief Binds lambdas to the socket's OnConnected, OnMessage, OnClosed, and OnConnectionError events. */
    void BindSocketHandlers();

    /**
     * @brief Schedules a reconnect attempt using exponential back-off.
     *
     * Does nothing if MaxRetryAttempts has been reached or the world is unavailable.
     */
    void ScheduleReconnect();

    /** @brief Cancels any pending reconnect timer. */
    void ClearReconnectTicker();

    /** @brief Timer callback that calls ConnectInternal() unless the service is being destroyed. */
    void HandleReconnectTick();
};
