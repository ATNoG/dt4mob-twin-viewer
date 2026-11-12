// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UMarcasPontosExtension, the type hooks for "marcas-pontos" (point road
 *        marking) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "MarcasPontosExtension.generated.h"

/**
 * @brief Type hooks for Infraestruturas de Portugal point road markings (type key
 *        "marcas-pontos", struct FInfPtMarcasPontosData, shown as "Road Marking (Point)").
 *
 * Only sets the info-tab fields and a green "MARKP" badge. Unlike the line and polygon
 * marking types, no geometry mesh component is attached.
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UMarcasPontosExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the marking code and position.
     * @return Rows for attributes.Code and attributes.location.latitude / longitude.
     */
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Code",      "attributes.Code"                },
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
     * @brief Fixed badge text, distinct from the line ("MARKL") and polygon ("MARKG") markings.
     * @param TypeKey  Unused.
     * @return         "MARKP".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("MARKP"); }
};
