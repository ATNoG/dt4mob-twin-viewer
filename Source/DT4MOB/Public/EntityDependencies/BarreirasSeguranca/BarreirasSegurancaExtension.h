// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UBarreirasSegurancaExtension, the type hooks for "barreiras-seguranca"
 *        (safety barrier) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "EntityDependencies/GeometryMesh/EntityGeometryMeshComponent.h"
#include "BarreirasSegurancaExtension.generated.h"

/**
 * @brief Type hooks for Infraestruturas de Portugal safety barriers (type key
 *        "barreiras-seguranca", struct FInfPtBarreirasSegurancaData, shown as "Safety Barrier").
 *
 * Sets the info-tab fields and an amber "BARRIER" badge, and attaches
 * UEntityGeometryMeshComponent so the barrier's attributes.geometry line can be drawn as a
 * mesh (mesh generation is currently off, see UEntityGeometryMeshComponent::bMeshGenerationEnabled).
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UBarreirasSegurancaExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the barrier length.
     * @return One "Length" row bound to attributes.shape_Length.
     */
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Length", "attributes.shape_Length" },
        };
    }
    /**
     * @brief Amber badge, the same colour as road signs.
     * @return Linear colour (0.859, 0.659, 0.337).
     */
    virtual FLinearColor GetBadgeColor() const override { return FLinearColor(0.859f, 0.659f, 0.337f); } // amber
    /**
     * @brief Fixed badge text.
     * @param TypeKey  Unused.
     * @return         "BARRIER".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("BARRIER"); }
    /**
     * @brief Attaches the generic geometry mesh component.
     * @return UEntityGeometryMeshComponent::StaticClass().
     */
    virtual TSubclassOf<UEntityBehaviorComponent> GetBehaviorComponentClass() const override { return UEntityGeometryMeshComponent::StaticClass(); }
};
