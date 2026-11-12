// Fill out your copyright notice in the Description page of Project Settings.
/**
 * @file
 * @brief Declares UTaludesExtension, the type hooks for "taludes" (slope and embankment) things.

 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "EntityDependencies/GeometryMesh/EntityGeometryMeshComponent.h"
#include "TaludesExtension.generated.h"

/**
 * @brief Type hooks for Infraestruturas de Portugal slopes and embankments (type key "taludes",
 *        struct FInfPtTaludesData).
 *
 * Info fields for length and area, an olive "TALUDE" badge, and UEntityGeometryMeshComponent
 * for the outline. Distinct from UTaludeExtension, which handles the older "muro-talude" things.
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UTaludesExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Length", "attributes.shape_Length" },
            { "Area",   "attributes.shape_Area"   },
        };
    }
    virtual FLinearColor GetBadgeColor() const override { return FLinearColor(0.643f, 0.741f, 0.278f); } // olive
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("TALUDE"); }
    virtual TSubclassOf<UEntityBehaviorComponent> GetBehaviorComponentClass() const override { return UEntityGeometryMeshComponent::StaticClass(); }
};
