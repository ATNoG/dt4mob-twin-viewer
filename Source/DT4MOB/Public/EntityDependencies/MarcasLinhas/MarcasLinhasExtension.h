// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "MarcasLinhasExtension.generated.h"

/** @brief Type-specific behavior for "marcas-linhas" (line road marking) entities. */
UCLASS()
class DT4MOB_API UMarcasLinhasExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Code",   "attributes.Code"         },
            { "Length", "attributes.SHAPE_Length" },
        };
    }
    virtual FLinearColor GetBadgeColor() const override { return FLinearColor(0.400f, 0.800f, 0.850f); } // cyan
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("MARKL"); }
};
