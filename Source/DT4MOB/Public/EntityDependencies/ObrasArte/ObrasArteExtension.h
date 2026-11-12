// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UObrasArteExtension, the type hooks for "obras-arte" (structural works such
 *        as bridges) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "EntityDependencies/GeometryMesh/EntityGeometryMeshComponent.h"
#include "ObrasArteExtension.generated.h"

/**
 * @brief Type hooks for Infraestruturas de Portugal structural works such as bridges (type
 *        key "obras-arte", struct FInfPtObrasArteData, shown as "Structural Work").
 *
 * Sets the info-tab fields and a cyan "STRUCT" badge, and attaches
 * UEntityGeometryMeshComponent so the structure's attributes.geometry footprint can be drawn
 * as a mesh (mesh generation is currently off, see UEntityGeometryMeshComponent::bMeshGenerationEnabled).
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UObrasArteExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the structure's shape length and area.
     * @return Rows for attributes.shape_Length and attributes.shape_Area.
     */
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Length", "attributes.shape_Length" },
            { "Area",   "attributes.shape_Area"   },
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
     * @return         "STRUCT".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("STRUCT"); }
    /**
     * @brief Attaches the generic geometry mesh component.
     * @return UEntityGeometryMeshComponent::StaticClass().
     */
    virtual TSubclassOf<UEntityBehaviorComponent> GetBehaviorComponentClass() const override { return UEntityGeometryMeshComponent::StaticClass(); }
};
