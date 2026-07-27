// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "GeoInstrumentExtension.generated.h"

/** @brief Type-specific behavior for ".instrument." (geo instrument) entities. */
UCLASS()
class DT4MOB_API UGeoInstrumentExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual TArray<FInfoField> GetDefaultInfoFields() const override;
    virtual FLinearColor GetBadgeColor() const override;
    virtual FString GetBadgeLabel(const FString& TypeKey) const override;
};
