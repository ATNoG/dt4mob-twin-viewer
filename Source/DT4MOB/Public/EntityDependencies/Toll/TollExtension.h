// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "TollExtension.generated.h"

/** @brief Type-specific behavior for "tolls:toll" (toll plaza) entities. */
UCLASS()
class DT4MOB_API UTollExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual FLinearColor GetBadgeColor() const override { return FLinearColor(0.557f, 0.882f, 0.533f); } // green
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("TOLL"); }
};
