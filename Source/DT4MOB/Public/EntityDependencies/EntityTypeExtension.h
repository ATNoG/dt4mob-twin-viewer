// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UEntityTypeExtension, the per-type hook object that the entity pipeline
 *        asks for badges, info fields, placement JSON and fetch/update policy.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "UI/InfoFieldTypes.h"
#include "Dom/JsonObject.h"
#include "EntityDependencies/EntityBehaviorComponent.h"
#include "EntityTypeExtension.generated.h"

/**
 * @brief Base class for per-type hooks that the generic Ditto-to-actor pipeline consults
 *        where registering a struct and display metadata is not enough.
 *
 * UDT4MOBEntityFactory creates one instance per registered type key and shares it across
 * every actor of that type, so an extension must not hold per-entity state. Per-actor state
 * belongs in a UEntityBehaviorComponent, which an extension names through
 * GetBehaviorComponentClass().
 *
 * Every hook has a default that gives the generic behaviour, so a subclass only overrides
 * what differs for its type. Subclasses live under
 * `Source/DT4MOB/Public/EntityDependencies/TYPE/` and are passed as the ExtensionClass
 * argument of UDT4MOBEntityFactory::RegisterType() in RegisterAllEntityTypes()
 * (Entities/EntityTypeRegistrations.cpp). Types registered without an extension class, and
 * lookups for unknown type keys, get a shared instance of this base class.
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UEntityTypeExtension : public UObject
{
    GENERATED_BODY()

public:
    /**
     * @brief Info-tab rows shown for this type until the user saves a custom field list.
     *
     * Read by UInfoFieldRegistry::GetDefaultFields(); a list the user saved for the type
     * takes precedence.
     *
     * @return Field list (label plus dot path into RawJson). The default is empty, which
     *         leaves the info panel blank.
     */
    virtual TArray<FInfoField> GetDefaultInfoFields() const { return {}; }

    /**
     * @brief Colour of the type badge in outline rows, association rows and entity windows.
     * @return Badge colour; the default is neutral gray (0.475, 0.475, 0.475).
     */
    virtual FLinearColor GetBadgeColor() const { return FLinearColor(0.475f, 0.475f, 0.475f); }

    /**
     * @brief Short uppercase text drawn in the type badge.
     * @param TypeKey  Registered type key the badge is for (for example "tolls:camera").
     * @return         Badge text; the default is the first 6 characters of TypeKey, uppercased.
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const { return TypeKey.Left(6).ToUpper(); }

    /**
     * @brief Builds the JSON body that AUnifiedController::LeftClick() PUTs to Ditto when the
     *        placement tool drops a new entity of this type.
     *
     * The controller also spawns a local actor from the same body before the PUT completes.
     * The default body has thingId set to `TypeKey:Guid`, policyId "dt4mob:default", and flat
     * attributes.latitude / attributes.longitude.
     *
     * @param TypeKey    Registered type key selected in the placement tool.
     * @param Guid       Freshly generated lowercase GUID for the new entity.
     * @param Lat        Placement latitude in degrees (WGS-84).
     * @param Lon        Placement longitude in degrees (WGS-84).
     * @param OutThingId Receives the full thingId assigned to the new entity.
     * @return           The thing JSON to PUT. The controller aborts the placement if this is invalid.
     */
    virtual TSharedPtr<FJsonObject> BuildPlacementJson(
        const FString& TypeKey, const FString& Guid, double Lat, double Lon, FString& OutThingId) const;

    /**
     * @brief Whether ADT4MOBGamemode::BeginPlay() loads every thing of this type once through
     *        UBackendConnectionService::GetAllThings(), ignoring tiles.
     *
     * Only consulted when tile streaming is disabled (ADT4MOBGamemode::bUseTileStreaming is
     * false). The filter comes from GetFullFetchFilter().
     *
     * @return True to request the one-shot full fetch; the default is false.
     */
    virtual bool RequiresFullFetch() const { return false; }

    /**
     * @brief Ditto RQL filter passed to GetAllThings() for the full fetch.
     *
     * Only consulted when RequiresFullFetch() returns true.
     *
     * @param TypeKey  Registered type key being fetched.
     * @return         RQL filter string; the default is empty (no filter).
     */
    virtual FString GetFullFetchFilter(const FString& TypeKey) const { return FString(); }

    /**
     * @brief Whether ATempUIActor tracks the gap between live WebSocket updates for actors of
     *        this type and logs (at Verbose level) when one arrives much later than expected.
     *
     * The threshold is three times the actor's rolling average update interval, with a 3 s
     * floor. Cached once in ATempUIActor::Initialize().
     *
     * @return True to enable the cadence check; the default is false.
     */
    virtual bool ShouldMonitorUpdateCadence() const { return false; }

    /**
     * @brief Staleness timeout for live entities of this type, in seconds.
     *
     * When positive, the actor ignores attributes.expiry_ts and is removed instead when no
     * WebSocket update for that specific thing arrives within this many seconds. The timer
     * restarts on every received message.
     *
     * Use this for sources that stamp a whole fleet with one shared expiry_ts (TRACI runs):
     * honouring it destroys every entity in the same frame when the deadline passes, even the
     * ones still being simulated. A per-entity timeout removes only the entities that actually
     * stopped updating, at different times.
     *
     * @return Timeout in seconds, or 0 (the default) to use expiry_ts through
     *         ATempUIActor::TryApplyExpiry().
     */
    virtual float LiveStalenessTimeoutSeconds() const { return 0.f; }

    /**
     * @brief Whether live patches for this type must re-sync the reflected StructInstance.
     *
     * Re-syncing runs FJsonObjectConverter::JsonObjectToUStruct() over the whole struct on
     * every full, attribute, leaf and feature-properties patch. Return false for
     * high-frequency types (simulated vehicles) whose position and info fields are read straight
     * from RawJson, so the rebuild is wasted work. The struct is still filled once in
     * ATempUIActor::Initialize().
     *
     * @return True (the default) to keep StructInstance in sync with RawJson.
     */
    virtual bool NeedsStructSyncOnLiveUpdate() const { return true; }

    /**
     * @brief Behaviour component class that ATempUIActor::Initialize() creates and registers on
     *        every actor of this type.
     *
     * Use this instead of the stateless hooks above when a type needs live per-instance state
     * (fetched data, loaded URLs, parsed geometry and so on).
     *
     * @return Component class, or nullptr (the default) for no component.
     */
    virtual TSubclassOf<UEntityBehaviorComponent> GetBehaviorComponentClass() const { return nullptr; }

    /**
     * @brief Whether ADT4MOBGamemode::HandleUnhandledThingMessage() may spawn an actor of this
     *        type from a live WebSocket event for a thing that is not in the world yet.
     *
     * By default entities only arrive through tile fetches, and WebSocket events for unknown
     * things are ignored. When this returns true, the game mode spawns straight from a full
     * "/" merge event, or otherwise fetches the snapshot with
     * UBackendConnectionService::GetThingById().
     *
     * Only opt in when things of this type are too short-lived or appear too often for the tile
     * fetch to catch (for example a sensor detection that expires within seconds). For other
     * types, the same unhandled event repeats on every message until the tile fetch catches up,
     * and spawning on demand risks duplicate or racing actor creation.
     *
     * @return True to allow on-demand spawning; the default is false.
     */
    virtual bool AllowsOnDemandSpawnFromWS() const { return false; }
};
