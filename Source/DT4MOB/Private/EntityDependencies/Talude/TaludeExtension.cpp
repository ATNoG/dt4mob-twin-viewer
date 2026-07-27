// Fill out your copyright notice in the Description page of Project Settings.

#include "EntityDependencies/Talude/TaludeExtension.h"

TArray<FInfoField> UTaludeExtension::GetDefaultInfoFields() const
{
    return {
        { "Latitude",  "attributes.location.latitude"  },
        { "Longitude", "attributes.location.longitude" },
    };
}

FLinearColor UTaludeExtension::GetBadgeColor() const
{
    return FLinearColor(0.557f, 0.882f, 0.533f); // green
}

FString UTaludeExtension::GetBadgeLabel(const FString& TypeKey) const
{
    return TEXT("SLOPE");
}
