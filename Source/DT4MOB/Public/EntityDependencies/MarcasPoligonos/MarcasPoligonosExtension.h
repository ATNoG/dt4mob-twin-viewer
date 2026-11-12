// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UMarcasPoligonosExtension, the type hooks for "marcas-poligonos" (polygon
 *        road marking) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "EntityDependencies/GeometryMesh/EntityGeometryMeshComponent.h"
#include "MarcasPoligonosExtension.generated.h"

/**
 * @brief Type hooks for Infraestruturas de Portugal polygon road markings (type key
 *        "marcas-poligonos", struct FInfPtMarcasPoligonosData, shown as
 *        "Road Marking (Polygon)").
 *
 * Sets the info-tab fields and a cyan "MARKG" badge, and attaches
 * UEntityGeometryMeshComponent so the marking's attributes.geometry ring can be drawn as a
 * filled mesh (mesh generation is currently off, see
 * UEntityGeometryMeshComponent::bMeshGenerationEnabled).
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UMarcasPoligonosExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the marking code, shape length and area.
     * @return Rows for attributes.Code, SHAPE_Length and SHAPE_Area.
     */
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Code",   "attributes.Code"         },
            { "Length", "attributes.SHAPE_Length" },
            { "Area",   "attributes.SHAPE_Area"   },
        };
    }
    /**
     * @brief Cyan badge, the colour shared by line and area geometry types.
     * @return Linear colour (0.400, 0.800, 0.850).
     */
    virtual FLinearColor GetBadgeColor() const override { return FLinearColor(0.400f, 0.800f, 0.850f); } // cyan
    /**
     * @brief Fixed badge text, distinct from the point ("MARKP") and line ("MARKL") markings.
     * @param TypeKey  Unused.
     * @return         "MARKG".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("MARKG"); }
    /**
     * @brief Attaches the generic geometry mesh component.
     * @return UEntityGeometryMeshComponent::StaticClass().
     */
    virtual TSubclassOf<UEntityBehaviorComponent> GetBehaviorComponentClass() const override { return UEntityGeometryMeshComponent::StaticClass(); }
};
