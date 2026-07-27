// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "GeoAssetExtension.generated.h"

/** @brief Type-specific behavior for "geo-asset" entities. */
UCLASS()
class DT4MOB_API UGeoAssetExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual TArray<FInfoField> GetDefaultInfoFields() const override;
    virtual FLinearColor GetBadgeColor() const override;
    virtual FString GetBadgeLabel(const FString& TypeKey) const override;
};
