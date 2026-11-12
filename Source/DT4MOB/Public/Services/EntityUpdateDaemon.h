/**
 * @file
 * @brief Declares UEntityUpdateDaemon, which routes live twin events to entity actors, and its delegates.
 * @ingroup services
 */
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dom/JsonObject.h"
#include "Services/WSService.h"
#include "Services/Backend/IBackendEnvelopeParser.h"
#include "EntityUpdateDaemon.generated.h"

class UWSService;

/**
 * @brief Fired on the GameThread for every Ditto twin event that matches a
 * registered thingId.
 *
 * @param Path        The Ditto path from the envelope, e.g.
 *                      "/"
 *                      "/attributes/location"
 *                      "/features/lidar-outsight/properties/1"
 *                    Lets the receiver decide which fields actually changed
 *                    without having to diff the JSON itself.
 *
 * @param ValueJson   The raw "value" from the envelope serialised back to a
 *                    JSON string. Its shape matches whatever Ditto puts under
 *                    the changed path:
 *                      path "/"                          -> full thing object
 *                      path "/attributes/..."            -> attribute subtree
 *                      path "/features/NAME/properties/1" -> property object
 * @ingroup services
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEntityUpdated,
                                             const FString &, Path,
                                             const FString &, ValueJson);

/**
 * @brief Fired when a Ditto WS event arrives for a thingId that has no registered actor.
 *
 * Listeners (e.g. the gamemode) can use this to fetch the thing from Ditto and
 * spawn an actor on demand, then call InjectUpdate() to replay the pending update.
 *
 * @param ThingId   Id of the thing the event is about.
 * @param Path      Ditto path of the change.
 * @param ValueJson New value as a JSON string.
 * @ingroup services
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnUnhandledThingMessage,
                                               const FString &, ThingId,
                                               const FString &, Path,
                                               const FString &, ValueJson);

/**
 * @brief GameInstance subsystem that routes live twin events from UWSService to the
 *        ATempUIActor registered for each thingId.
 *
 * On Initialize() it binds to UWSService's delegates. Every incoming message is parsed by an
 * IBackendEnvelopeParser into (thingId, path, value) and dispatched to the delegates
 * registered for that thingId. Events for unknown things go to OnUnhandledThingMessage.
 *
 * UWSService opens the socket by itself after login; StartConnection() and StopConnection()
 * are an alternative manual entry point that the viewer does not currently use.
 *
 * Actors call RegisterEntity() / UnregisterEntity() in BeginPlay / EndPlay.
 * @ingroup services
 */
UCLASS()
class DT4MOB_API UEntityUpdateDaemon : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ------------------------------------------------------------------ //
    //  Subsystem lifecycle
    // ------------------------------------------------------------------ //
    /**
     * @brief Creates the Ditto envelope parser and binds to UWSService's delegates.
     * @param Collection Subsystem collection, used to make sure UWSService initialises first.
     */
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;

    /** @brief Unbinds from UWSService, disconnects it if StartConnection() was used, and clears all registrations. */
    virtual void Deinitialize() override;

    // ------------------------------------------------------------------ //
    //  Connection API
    // ------------------------------------------------------------------ //

    /**
     * @brief Asks UWSService to connect with the given settings.
     * @param InUrl                   WebSocket URL.
     * @param InAuthHeader            Authorization header value.
     * @param InStartMessage          Message sent right after the socket opens.
     * @param bInAutoReconnect        True to reconnect after an unexpected close.
     * @param InReconnectDelaySeconds Base delay between reconnect attempts, in seconds.
     */
    UFUNCTION(BlueprintCallable, Category = "EntityUpdateDaemon")
    void StartConnection(const FString &InUrl,
                         const FString &InAuthHeader,
                         const FString &InStartMessage,
                         bool bInAutoReconnect = true,
                         float InReconnectDelaySeconds = 5.0f);

    /** @brief Closes the socket and stops reconnection attempts. */
    UFUNCTION(BlueprintCallable, Category = "EntityUpdateDaemon")
    void StopConnection();

    // ------------------------------------------------------------------ //
    //  Entity registration  (called by ATempUIActor)
    // ------------------------------------------------------------------ //

    /**
     * @brief Starts routing events for ThingId to Delegate. Registering the same delegate twice has no effect.
     * @param ThingId  Thing to listen for.
     * @param Delegate Delegate to call. The daemon keeps a raw pointer, so the owner must unregister before it is destroyed.
     */
    void RegisterEntity(const FString &ThingId, FOnEntityUpdated &Delegate);

    /**
     * @brief Stops routing events for ThingId to Delegate.
     * @param ThingId  Thing passed to RegisterEntity().
     * @param Delegate Delegate passed to RegisterEntity().
     */
    void UnregisterEntity(const FString &ThingId, FOnEntityUpdated &Delegate);

    /**
     * @brief Dispatches a (path, valueJson) update to any actor registered for ThingId.
     *
     * Used by the gamemode's on-demand spawn path: after fetching and spawning a new actor
     * (which registers itself in BeginPlay), call this to replay the WS update that
     * triggered the spawn so the actor gets the freshest data immediately.
     *
     * @param ThingId   Thing the update is for.
     * @param Path      Ditto path of the change.
     * @param ValueJson New value as a JSON string.
     */
    void InjectUpdate(const FString &ThingId, const FString &Path, const FString &ValueJson);

    /**
     * @brief True while the WebSocket is connected AND a Ditto message arrived within
     *        StaleAfterSeconds. When false the event stream has stalled (socket flap,
     *        Ditto/connector hiccup): callers enforcing per-entity TTL should freeze
     *        expiry until it recovers, otherwise a whole fleet sharing one expiry_ts is
     *        destroyed at once during what is really a transient backend outage.
     * @param StaleAfterSeconds How long without a message counts as stalled.
     * @return True if the stream is live.
     */
    bool IsStreamHealthy(float StaleAfterSeconds = 5.0f) const;

    // ------------------------------------------------------------------ //
    //  Observable socket-level events  (for HUD / connection indicators)
    // ------------------------------------------------------------------ //
    /** @brief Fired when a WS message arrives for a thingId with no registered actor. */
    UPROPERTY(BlueprintAssignable, Category = "EntityUpdateDaemon")
    FOnUnhandledThingMessage OnUnhandledThingMessage;

    /** @brief Re-broadcast of UWSService::OnConnected. */
    UPROPERTY(BlueprintAssignable, Category = "EntityUpdateDaemon")
    FWSServiceConnected OnSocketConnected;

    /** @brief Re-broadcast of UWSService::OnClosed. */
    UPROPERTY(BlueprintAssignable, Category = "EntityUpdateDaemon")
    FWSServiceClosed OnSocketClosed;

    /** @brief Re-broadcast of UWSService::OnError. */
    UPROPERTY(BlueprintAssignable, Category = "EntityUpdateDaemon")
    FWSServiceError OnSocketError;

private:
    // ---- Socket event handlers (bound directly to UWSService) ----
    /** @brief Broadcasts OnSocketConnected. */
    UFUNCTION()
    void HandleSocketConnected();

    /**
     * @brief Parses one message, dispatches it, and logs throughput once per second.
     * @param Message Raw text frame.
     */
    UFUNCTION()
    void HandleSocketMessage(const FString &Message);

    /**
     * @brief Broadcasts OnSocketError.
     * @param Error Error text from UWSService.
     */
    UFUNCTION()
    void HandleSocketError(const FString &Error);

    /**
     * @brief Broadcasts OnSocketClosed.
     * @param StatusCode WebSocket close code.
     * @param Reason     Close reason.
     */
    UFUNCTION()
    void HandleSocketClosed(int32 StatusCode, const FString &Reason);

    /**
     * @brief Calls every delegate registered for ThingId, or fires OnUnhandledThingMessage if there is none.
     * @param ThingId   Thing the event is about.
     * @param Path      Ditto path of the change.
     * @param ValueJson New value as a JSON string.
     */
    void DispatchUpdate(const FString &ThingId,
                        const FString &Path,
                        const FString &ValueJson);

    // ---- State ----
    /** @brief Registered delegates per thingId. Raw pointers into the owning actors. */
    TMap<FString, TArray<FOnEntityUpdated *>> EntityDelegates;

    /** @brief FPlatformTime::Seconds() when the last WS message was received (0 = none yet). */
    double LastMessageRealTime = 0.0;

    // ---- Throughput instrumentation (logged once per second) ----
    /** @brief FPlatformTime::Seconds() at the start of the current 1s stats window. */
    double StatWindowStart = 0.0;
    /** @brief Messages received+dispatched in the current window. */
    int32 StatMsgCount = 0;
    /** @brief Wall-clock seconds spent in parse+dispatch in the current window. */
    double StatProcSeconds = 0.0;
    /** @brief Largest single parse+dispatch cost seen this window, in seconds. */
    double StatMaxMsgSeconds = 0.0;

    /** @brief The WebSocket service this daemon listens to. */
    UPROPERTY()
    UWSService *WSService = nullptr;

    /** @brief The one place this class knows the backend's live-update envelope shape.
     *  Everything else here (registration, dispatch, stream-health) is generic routing. */
    TUniquePtr<IBackendEnvelopeParser> Parser;

    FString Url;                        ///< URL from the last StartConnection().
    FString AuthHeader;                 ///< Authorization header from the last StartConnection().
    FString StartMessage;               ///< Start message from the last StartConnection().
    bool bAutoReconnect = true;         ///< Reconnect setting from the last StartConnection().
    float ReconnectDelaySeconds = 5.0f; ///< Reconnect delay from the last StartConnection().
    bool bConnectionRequested = false;  ///< True after StartConnection() until StopConnection(); Deinitialize() then disconnects.
};