// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "EntityDependencies/GeometryMesh/EntityGeometryMeshComponent.h"
#include "TaludesExtension.generated.h"

/** @brief Type-specific behavior for "taludes" (GIS-sourced slope/embankment) entities.
 *  Distinct from UTaludeExtension, which handles the older "muro-talude" Equivia schema. */
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
