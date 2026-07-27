// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "TaludeExtension.generated.h"

/** @brief Type-specific behavior for "muro-talude" (slope) entities. */
UCLASS()
class DT4MOB_API UTaludeExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual TArray<FInfoField> GetDefaultInfoFields() const override;
    virtual FLinearColor GetBadgeColor() const override;
    virtual FString GetBadgeLabel(const FString& TypeKey) const override;
};
