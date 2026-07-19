// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "IgnitionPointExtension.generated.h"

/** @brief Type-specific behavior for "fire:" (ignition point) entities. */
UCLASS()
class DT4MOB_API UIgnitionPointExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual TArray<FInfoField> GetDefaultInfoFields() const override;
    virtual FLinearColor GetBadgeColor() const override;
    virtual FString GetBadgeLabel(const FString& TypeKey) const override;
    virtual TSharedPtr<FJsonObject> BuildPlacementJson(
        const FString& TypeKey, const FString& Guid, double Lat, double Lon, FString& OutThingId) const override;
};
