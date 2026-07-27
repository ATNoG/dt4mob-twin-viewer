// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "PavimentosExtension.generated.h"

/** @brief Type-specific behavior for "pavimentos" (pavement) entities. */
UCLASS()
class DT4MOB_API UPavimentosExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Length", "attributes.shape_Length" },
        };
    }
    virtual FLinearColor GetBadgeColor() const override { return FLinearColor(0.400f, 0.800f, 0.850f); } // cyan
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("PAVE"); }
};
