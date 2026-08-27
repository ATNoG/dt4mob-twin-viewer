// Fill out your copyright notice in the Description page of Project Settings.

#include "EntityDependencies/IgnitionPoint/IgnitionPointExtension.h"
#include "EntityStructs/IgnitionPointStruct.h"
#include "JsonObjectConverter.h"

TArray<FInfoField> UIgnitionPointExtension::GetDefaultInfoFields() const
{
    return {
        { "State",     "attributes.state"             },
        { "Latitude",  "attributes.fire_ignition.lat" },
        { "Longitude", "attributes.fire_ignition.lon" },
    };
}

FLinearColor UIgnitionPointExtension::GetBadgeColor() const
{
    return FLinearColor(1.000f, 0.380f, 0.180f); // orange
}

FString UIgnitionPointExtension::GetBadgeLabel(const FString& TypeKey) const
{
    return TEXT("FIRE");
}

TSharedPtr<FJsonObject> UIgnitionPointExtension::BuildPlacementJson(
    const FString& TypeKey, const FString& Guid, double Lat, double Lon, FString& OutThingId) const
{
    FIgnitionPointData IgnitionPoint = FIgnitionPointData::MakeDefault(Lat, Lon);
    OutThingId = TEXT("fire:") + Guid;
    IgnitionPoint.thingId = OutThingId;

    TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
    FJsonObjectConverter::UStructToJsonObject(FIgnitionPointData::StaticStruct(), &IgnitionPoint, Body.ToSharedRef(), 0, 0);
    return Body;
}
