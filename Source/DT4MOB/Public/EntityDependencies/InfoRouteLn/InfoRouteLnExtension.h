// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UInfoRouteLnExtension, the type hooks for "info-route-ln" (route line) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "EntityDependencies/GeometryMesh/EntityGeometryMeshComponent.h"
#include "InfoRouteLnExtension.generated.h"

/**
 * @brief Type hooks for Infraestruturas de Portugal route lines (type key "info-route-ln",
 *        struct FInfPtInfoRouteLnData, shown as "Route Info (Line)").
 *
 * Sets the info-tab fields and a cyan "ROUTE" badge, and attaches
 * UEntityGeometryMeshComponent so the route's attributes.geometry line can be drawn as a mesh
 * (mesh generation is currently off, see UEntityGeometryMeshComponent::bMeshGenerationEnabled).
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UInfoRouteLnExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the route name, road number, start and end kilometre and length.
     * @return Rows for attributes.route_name, roadnumber, kmi, kmf and Shape_Length.
     */
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Route",      "attributes.route_name" },
            { "Road No.",   "attributes.roadnumber" },
            { "Km Start",   "attributes.kmi"        },
            { "Km End",     "attributes.kmf"        },
            { "Length",     "attributes.Shape_Length" },
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
     * @return         "ROUTE".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("ROUTE"); }
    /**
     * @brief Attaches the generic geometry mesh component.
     * @return UEntityGeometryMeshComponent::StaticClass().
     */
    virtual TSubclassOf<UEntityBehaviorComponent> GetBehaviorComponentClass() const override { return UEntityGeometryMeshComponent::StaticClass(); }
};
