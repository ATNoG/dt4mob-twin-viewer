// Fill out your copyright notice in the Description page of Project Settings.

#include "EntityDependencies/Barrier/BarrierExtension.h"

TArray<FInfoField> UBarrierExtension::GetDefaultInfoFields() const
{
    return {
        { "Latitude",  "attributes.location.latitude"  },
        { "Longitude", "attributes.location.longitude" },
    };
}

FLinearColor UBarrierExtension::GetBadgeColor() const
{
    return FLinearColor(0.859f, 0.659f, 0.337f); // amber
}

FString UBarrierExtension::GetBadgeLabel(const FString& TypeKey) const
{
    return TEXT("BARRIER");
}
