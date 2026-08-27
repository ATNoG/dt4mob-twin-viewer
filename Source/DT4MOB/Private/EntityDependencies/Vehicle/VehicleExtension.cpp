// Fill out your copyright notice in the Description page of Project Settings.

#include "EntityDependencies/Vehicle/VehicleExtension.h"

TArray<FInfoField> UVehicleExtension::GetDefaultInfoFields() const
{
    return {
        { "Speed",        "features.state.properties.speed"     },
        { "Heading",      "features.state.properties.angle"     },
        { "Latitude",     "features.state.properties.latitude"  },
        { "Longitude",    "features.state.properties.longitude" },
        { "Acceleration", "features.state.properties.accel"     },
        { "TTL",          "attributes.time_to_live"             },
    };
}

FLinearColor UVehicleExtension::GetBadgeColor() const
{
    return FLinearColor(0.557f, 0.714f, 1.000f); // blue
}

FString UVehicleExtension::GetBadgeLabel(const FString& TypeKey) const
{
    return TEXT("CAR");
}
