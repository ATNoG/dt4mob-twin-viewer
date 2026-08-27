// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "MarcosQuilometricosExtension.generated.h"

/** @brief Type-specific behavior for "marcos-quilometricos" (km marker) entities. */
UCLASS()
class DT4MOB_API UMarcosQuilometricosExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Latitude",  "attributes.location.latitude"  },
            { "Longitude", "attributes.location.longitude" },
        };
    }
    virtual FLinearColor GetBadgeColor() const override { return FLinearColor(0.557f, 0.882f, 0.533f); } // green
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("KM"); }
};
