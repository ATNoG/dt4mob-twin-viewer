// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UIgnitionPointExtension, the type hooks for "fire:" (wildfire ignition
 *        point) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "EntityDependencies/IgnitionPoint/FireBehaviorComponent.h"
#include "IgnitionPointExtension.generated.h"

/**
 * @brief Type hooks for wildfire ignition points (type key "fire:", struct
 *        FIgnitionPointData, shown as "Ignition Point").
 *
 * Sets the info-tab fields and an orange "FIRE" badge, builds a full FIgnitionPointData body
 * when the user places a new fire, and attaches UFireBehaviorComponent, which loads the fire
 * cone and simulation models and supplies terrain-exclusion polygons.
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UIgnitionPointExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the fire state and ignition position.
     * @return Rows for attributes.state and attributes.fire_ignition.lat / lon.
     */
    virtual TArray<FInfoField> GetDefaultInfoFields() const override;
    /**
     * @brief Orange badge.
     * @return Linear colour (1.000, 0.380, 0.180).
     */
    virtual FLinearColor GetBadgeColor() const override;
    /**
     * @brief Fixed badge text.
     * @param TypeKey  Unused.
     * @return         "FIRE".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override;
    /**
     * @brief Builds a new ignition point from FIgnitionPointData::MakeDefault().
     *
     * The body carries attributes.fire_ignition at the clicked position, the matching geotile
     * and an expiry_ts 24 hours from now. The thingId is always `fire:Guid`, whatever TypeKey
     * was selected.
     *
     * @param TypeKey    Unused; the "fire:" prefix is hard-coded.
     * @param Guid       Freshly generated GUID for the new fire.
     * @param Lat        Ignition latitude in degrees.
     * @param Lon        Ignition longitude in degrees.
     * @param OutThingId Receives `fire:Guid`.
     * @return           The serialised FIgnitionPointData.
     */
    virtual TSharedPtr<FJsonObject> BuildPlacementJson(
        const FString& TypeKey, const FString& Guid, double Lat, double Lon, FString& OutThingId) const override;
    /**
     * @brief Attaches the fire model and perimeter component.
     * @return UFireBehaviorComponent::StaticClass().
     */
    virtual TSubclassOf<UEntityBehaviorComponent> GetBehaviorComponentClass() const override { return UFireBehaviorComponent::StaticClass(); }
};
