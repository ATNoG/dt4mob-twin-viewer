// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UMeteoExtension, the type hooks for "meteo" (weather station) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "MeteoExtension.generated.h"

/**
 * @brief Type hooks for weather stations (type key "meteo", struct FMeteorologyData, shown as
 *        "Meteo Station").
 *
 * Only sets the info-tab fields (station name, position and the latest readings from
 * features.meteorology) and a purple "METEO" badge.
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UMeteoExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the station name and position plus the current weather readings.
     * @return Rows for attributes.location_name and attributes.location, followed by
     *         temperature, humidity, wind intensity and direction, pressure, accumulated
     *         precipitation and reading time from features.meteorology.properties.
     */
    virtual TArray<FInfoField> GetDefaultInfoFields() const override;
    /**
     * @brief Purple badge.
     * @return Linear colour (0.800, 0.533, 1.000).
     */
    virtual FLinearColor GetBadgeColor() const override;
    /**
     * @brief Fixed badge text.
     * @param TypeKey  Unused.
     * @return         "METEO".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override;
};
