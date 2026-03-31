#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dom/JsonObject.h"
#include "Services/WSService.h"
#include "EntityUpdateDaemon.generated.h"

class UWSService;

/**
 * Fired on the GameThread for every Ditto twin event that matches a
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
 *                      path "/"                          → full thing object
 *                      path "/attributes/..."            → attribute subtree
 *                      path "/features/<f>/properties/1" → property object
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEntityUpdated,
                                             const FString &, Path,
                                             const FString &, ValueJson);

/**
 * UEntityUpdateDaemon
 *
 * GameInstance subsystem that owns the WebSocket connection and routes
 * Ditto twin events to registered ATempUIActor instances.
 *
 * Responsibilities:
 *   - Acquire UWSService on Initialize(), bind all socket delegates.
 *   - Parse the Ditto protocol envelope (topic → thingId, path, value).
 *   - Dispatch per-thingId events carrying (path, valueJson) to actors.
 *   - Tear down cleanly on Deinitialize().
 *
 * Actors call RegisterEntity() / UnregisterEntity() in BeginPlay / EndPlay.
 */
UCLASS()
class DT4MOB_API UEntityUpdateDaemon : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ------------------------------------------------------------------ //
    //  Subsystem lifecycle
    // ------------------------------------------------------------------ //
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    // ------------------------------------------------------------------ //
    //  Connection API
    // ------------------------------------------------------------------ //

    /** Open the WebSocket. Idempotent while already connected. */
    UFUNCTION(BlueprintCallable, Category = "EntityUpdateDaemon")
    void StartConnection(const FString &InUrl,
                         const FString &InAuthHeader,
                         const FString &InStartMessage,
                         bool bInAutoReconnect = true,
                         float InReconnectDelaySeconds = 5.0f);

    /** Close the socket and stop reconnection attempts. */
    UFUNCTION(BlueprintCallable, Category = "EntityUpdateDaemon")
    void StopConnection();

    // ------------------------------------------------------------------ //
    //  Entity registration  (called by ATempUIActor)
    // ------------------------------------------------------------------ //

    void RegisterEntity(const FString &ThingId, FOnEntityUpdated &Delegate);
    void UnregisterEntity(const FString &ThingId, FOnEntityUpdated &Delegate);

    // ------------------------------------------------------------------ //
    //  Observable socket-level events  (for HUD / connection indicators)
    // ------------------------------------------------------------------ //
    UPROPERTY(BlueprintAssignable, Category = "EntityUpdateDaemon")
    FWSServiceConnected OnSocketConnected;

    UPROPERTY(BlueprintAssignable, Category = "EntityUpdateDaemon")
    FWSServiceClosed OnSocketClosed;

    UPROPERTY(BlueprintAssignable, Category = "EntityUpdateDaemon")
    FWSServiceError OnSocketError;

private:
    // ---- Socket event handlers (bound directly to UWSService) ----
    UFUNCTION()
    void HandleSocketConnected();
    UFUNCTION()
    void HandleSocketMessage(const FString &Message);
    UFUNCTION()
    void HandleSocketError(const FString &Error);
    UFUNCTION()
    void HandleSocketClosed(int32 StatusCode, const FString &Reason);

    // ---- Ditto parsing ----

    /**
     * Parse one Ditto envelope. Returns true on success.
     *
     * Fills:
     *   OutThingId   — "namespace:thingName"  (e.g. "tolls:toll-1")
     *   OutPath      — the Ditto path field    (e.g. "/features/lidar-outsight/properties/1")
     *   OutValueJson — the "value" field serialised as a JSON string
     */
    bool ParseDittoMessage(const FString &RawMessage,
                           FString &OutThingId,
                           FString &OutPath,
                           FString &OutValueJson) const;

    void DispatchUpdate(const FString &ThingId,
                        const FString &Path,
                        const FString &ValueJson);

    // ---- State ----
    TMap<FString, TArray<FOnEntityUpdated *>> EntityDelegates;

    UPROPERTY()
    UWSService *WSService = nullptr;

    FString Url;
    FString AuthHeader;
    FString StartMessage;
    bool bAutoReconnect = true;
    float ReconnectDelaySeconds = 5.0f;
    bool bConnectionRequested = false;
};