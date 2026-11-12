// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UMarcasLinhasExtension, the type hooks for "marcas-linhas" (line road
 *        marking) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "EntityDependencies/GeometryMesh/EntityGeometryMeshComponent.h"
#include "MarcasLinhasExtension.generated.h"

/**
 * @brief Type hooks for Infraestruturas de Portugal line road markings (type key
 *        "marcas-linhas", struct FInfPtMarcasLinhasData, shown as "Road Marking (Line)").
 *
 * Sets the info-tab fields and a cyan "MARKL" badge, and attaches
 * UEntityGeometryMeshComponent so the marking's attributes.geometry line can be drawn as a
 * mesh (mesh generation is currently off, see UEntityGeometryMeshComponent::bMeshGenerationEnabled).
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UMarcasLinhasExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the marking code and length.
     * @return Rows for attributes.Code and attributes.SHAPE_Length.
     */
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Code",   "attributes.Code"         },
            { "Length", "attributes.SHAPE_Length" },
        };
    }
    /**
     * @brief Cyan badge, the colour shared by line and area geometry types.
     * @return Linear colour (0.400, 0.800, 0.850).
     */
    virtual FLinearColor GetBadgeColor() const override { return FLinearColor(0.400f, 0.800f, 0.850f); } // cyan
    /**
     * @brief Fixed badge text, distinct from the point ("MARKP") and polygon ("MARKG") markings.
     * @param TypeKey  Unused.
     * @return         "MARKL".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("MARKL"); }
    /**
     * @brief Attaches the generic geometry mesh component.
     * @return UEntityGeometryMeshComponent::StaticClass().
     */
    virtual TSubclassOf<UEntityBehaviorComponent> GetBehaviorComponentClass() const override { return UEntityGeometryMeshComponent::StaticClass(); }
};
