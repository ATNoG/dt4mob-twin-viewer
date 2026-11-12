// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UIluminacaoExtension, the type hooks for "iluminacao" (street lighting) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "IluminacaoExtension.generated.h"

/**
 * @brief Type hooks for Infraestruturas de Portugal street lighting points (type key
 *        "iluminacao", struct FInfPtIluminacaoData, shown as "IP Lighting").
 *
 * Only sets the info-tab fields and a green "ILUM" badge. The model comes from the generic
 * attributes.polygon GLB load in ATempUIActor.
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UIluminacaoExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the lighting point's state, road and position.
     * @return Rows for attributes.estado, attributes.via and attributes.location.
     */
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Estado",    "attributes.estado"             },
            { "Via",       "attributes.via"                },
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
     * @return         "ILUM".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("ILUM"); }
};
