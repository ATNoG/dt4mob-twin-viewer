// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UCoordenadasProjetoAprovadasExtension, the type hooks for
 *        "coordenadas-de-projeto-aprovadas-ip" (approved project coordinate) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "CoordenadasProjetoAprovadasExtension.generated.h"

/**
 * @brief Type hooks for Infraestruturas de Portugal approved project coordinate points (type
 *        key "coordenadas-de-projeto-aprovadas-ip", struct
 *        FInfPtCoordenadasProjetoAprovadasData, shown as "Approved Project Coordinate").
 *
 * Only sets the info-tab fields and a green "COORD" badge; everything else uses the
 * base-class defaults.
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UCoordenadasProjetoAprovadasExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the point position.
     * @return "Latitude" and "Longitude" rows bound to attributes.location.
     */
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Latitude",  "attributes.location.latitude"  },
            { "Longitude", "attributes.location.longitude" },
        };
    }
    /**
     * @brief Green badge, the colour shared by point-like infrastructure types.
     * @return Linear colour (0.557, 0.882, 0.533).
     */
    virtual FLinearColor GetBadgeColor() const override { return FLinearColor(0.557f, 0.882f, 0.533f); } // green
    /**
     * @brief Fixed badge text.
     * @param TypeKey  Unused.
     * @return         "COORD".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("COORD"); }
};
