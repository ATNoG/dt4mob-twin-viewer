// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UPorticosExtension, the type hooks for "porticos" (gantry) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "PorticosExtension.generated.h"

/**
 * @brief Type hooks for Infraestruturas de Portugal gantries over the road (type key
 *        "porticos", struct FInfPtPorticosData, shown as "Gantry").
 *
 * Only sets the info-tab fields and a green "GANTRY" badge.
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UPorticosExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the gantry position.
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
     * @return         "GANTRY".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("GANTRY"); }
};
