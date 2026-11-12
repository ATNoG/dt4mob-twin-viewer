// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UPavimentosExtension, the type hooks for "pavimentos" (pavement section) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "EntityDependencies/GeometryMesh/EntityGeometryMeshComponent.h"
#include "PavimentosExtension.generated.h"

/**
 * @brief Type hooks for Infraestruturas de Portugal pavement sections (type key "pavimentos",
 *        struct FInfPtPavimentosData, shown as "Pavement").
 *
 * Sets the info-tab fields and a cyan "PAVE" badge, and attaches
 * UEntityGeometryMeshComponent so the section's attributes.geometry can be drawn as a mesh
 * (mesh generation is currently off, see UEntityGeometryMeshComponent::bMeshGenerationEnabled).
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UPavimentosExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the pavement section length.
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
     * @return         "PAVE".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("PAVE"); }
    /**
     * @brief Attaches the generic geometry mesh component.
     * @return UEntityGeometryMeshComponent::StaticClass().
     */
    virtual TSubclassOf<UEntityBehaviorComponent> GetBehaviorComponentClass() const override { return UEntityGeometryMeshComponent::StaticClass(); }
};
