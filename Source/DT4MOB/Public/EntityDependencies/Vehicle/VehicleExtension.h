// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "VehicleExtension.generated.h"

/** @brief Type-specific behavior for "traci" (vehicle) entities. */
UCLASS()
class DT4MOB_API UVehicleExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual TArray<FInfoField> GetDefaultInfoFields() const override;
    virtual FLinearColor GetBadgeColor() const override;
    virtual FString GetBadgeLabel(const FString& TypeKey) const override;
    virtual bool ShouldMonitorUpdateCadence() const override { return true; }
};
