// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UDrenagemLinearExtension, the type hooks for "drenagem-linear"
 *        (linear drainage) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "EntityDependencies/GeometryMesh/EntityGeometryMeshComponent.h"
#include "DrenagemLinearExtension.generated.h"

/**
 * @brief Type hooks for Infraestruturas de Portugal linear drainage runs (type key
 *        "drenagem-linear", struct FInfPtDrenagemLinearData, shown as "Linear Drainage").
 *
 * Sets the info-tab fields and a cyan "DREN" badge, and attaches
 * UEntityGeometryMeshComponent so the attributes.geometry line can be drawn as a mesh (mesh
 * generation is currently off, see UEntityGeometryMeshComponent::bMeshGenerationEnabled).
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UDrenagemLinearExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the drainage run length.
     * @return One "Length" row bound to attributes.shape_Length.
     */
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Length", "attributes.shape_Length" },
        };
    }
    /**
     * @brief Cyan badge, the colour shared by line and area geometry types.
     * @return Linear colour (0.400, 0.800, 0.850).
     */
    virtual FLinearColor GetBadgeColor() const override { return FLinearColor(0.400f, 0.800f, 0.850f); } // cyan
    /**
     * @brief Fixed badge text.
     * @param TypeKey  Unused.
     * @return         "DREN".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("DREN"); }
    /**
     * @brief Attaches the generic geometry mesh component.
     * @return UEntityGeometryMeshComponent::StaticClass().
     */
    virtual TSubclassOf<UEntityBehaviorComponent> GetBehaviorComponentClass() const override { return UEntityGeometryMeshComponent::StaticClass(); }
};
