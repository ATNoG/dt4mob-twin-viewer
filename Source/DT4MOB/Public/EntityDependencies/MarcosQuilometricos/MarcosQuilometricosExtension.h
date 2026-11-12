// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UMarcosQuilometricosExtension, the type hooks for "marcos-quilometricos"
 *        (kilometre marker) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "MarcosQuilometricosExtension.generated.h"

/**
 * @brief Type hooks for Infraestruturas de Portugal kilometre markers (type key
 *        "marcos-quilometricos", struct FInfPtMarcosQuilometricosData, shown as "Km Marker").
 *
 * Only sets the info-tab fields and a green "KM" badge.
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UMarcosQuilometricosExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the marker position.
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
     * @return         "KM".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("KM"); }
};
