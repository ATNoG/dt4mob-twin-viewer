// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/InfoFieldTypes.h"
#include "Dom/JsonObject.h"
#include "EntityTypeExtension.generated.h"

/**
 * @brief Base class for optional, per-entity-type behavior that the generic Ditto→actor
 *        pipeline cannot express through UDT4MOBEntityFactory's ThingStructMap/TypeMetaMap
 *        registration alone.
 *
 * Every hook below has a default matching the pipeline's previous generic fallback
 * behavior. A type only needs a subclass — placed under
 * Source/DT4MOB/Public/EntityDependencies/<TypeName>/ — if it needs to override one of
 * these; most registered types need no subclass at all and just use this base directly.
 *
 * Register a subclass for a type via UDT4MOBEntityFactory's constructor, alongside its
 * existing Register() call (see UDT4MOBEntityFactory::UDT4MOBEntityFactory()).
 */
UCLASS()
class DT4MOB_API UEntityTypeExtension : public UObject
{
    GENERATED_BODY()

public:
    /** @brief Info-tab field list shown for this type by default. Default: none (empty panel). */
    virtual TArray<FInfoField> GetDefaultInfoFields() const { return {}; }

    /** @brief Badge color used in outline rows / entity windows. Default: neutral gray. */
    virtual FLinearColor GetBadgeColor() const { return FLinearColor(0.475f, 0.475f, 0.475f); }

    /** @brief Badge label (short, uppercase). Default: first 6 characters of the type key, uppercased. */
    virtual FString GetBadgeLabel(const FString& TypeKey) const { return TypeKey.Left(6).ToUpper(); }

    /**
     * @brief Builds the JSON body PUT to Ditto when a new entity of this type is placed in
     *        the world (see AUnifiedController::LeftClick).
     *
     * Default: minimal shape with thingId = "<TypeKey>:<Guid>" and flat
     * attributes.latitude/longitude.
     *
     * @param TypeKey    Registered type key selected in the placement tool.
     * @param Guid       Freshly generated GUID for the new entity.
     * @param Lat        Placement latitude.
     * @param Lon        Placement longitude.
     * @param OutThingId Set to the full thingId assigned to the new entity.
     */
    virtual TSharedPtr<FJsonObject> BuildPlacementJson(
        const FString& TypeKey, const FString& Guid, double Lat, double Lon, FString& OutThingId) const;

    /**
     * @brief If true, ADT4MOBGamemode::BeginPlay fetches every thing of this type once via
     *        GetAllThings(), regardless of tile/geotile, instead of relying on tile streaming
     *        to pick it up. Only consulted when tile streaming is disabled
     *        (see ADT4MOBGamemode::bUseTileStreaming). Default: false.
     */
    virtual bool RequiresFullFetch() const { return false; }

    /** @brief Ditto RQL filter used for the full fetch above. Only consulted if RequiresFullFetch(). */
    virtual FString GetFullFetchFilter(const FString& TypeKey) const { return FString(); }

    /**
     * @brief Whether ATempUIActor should monitor WS update cadence for actors of this type
     *        and log a stall warning if updates lag further than expected. Default: false.
     */
    virtual bool ShouldMonitorUpdateCadence() const { return false; }
};
