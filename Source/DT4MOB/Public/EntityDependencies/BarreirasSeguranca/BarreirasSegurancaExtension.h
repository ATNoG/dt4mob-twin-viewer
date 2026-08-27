// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "EntityDependencies/GeometryMesh/EntityGeometryMeshComponent.h"
#include "BarreirasSegurancaExtension.generated.h"

/** @brief Type-specific behavior for "barreiras-seguranca" (safety barrier) entities. */
UCLASS()
class DT4MOB_API UBarreirasSegurancaExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Length", "attributes.shape_Length" },
        };
    }
    virtual FLinearColor GetBadgeColor() const override { return FLinearColor(0.859f, 0.659f, 0.337f); } // amber
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("BARRIER"); }
    virtual TSubclassOf<UEntityBehaviorComponent> GetBehaviorComponentClass() const override { return UEntityGeometryMeshComponent::StaticClass(); }
};
