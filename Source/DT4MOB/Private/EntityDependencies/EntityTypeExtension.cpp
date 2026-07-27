// Fill out your copyright notice in the Description page of Project Settings.

#include "EntityDependencies/EntityTypeExtension.h"

TSharedPtr<FJsonObject> UEntityTypeExtension::BuildPlacementJson(
    const FString& TypeKey, const FString& Guid, double Lat, double Lon, FString& OutThingId) const
{
    OutThingId = TypeKey + TEXT(":") + Guid;

    TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
    Body->SetStringField(TEXT("thingId"), OutThingId);
    Body->SetStringField(TEXT("policyId"), TEXT("dt4mob:default"));

    TSharedPtr<FJsonObject> Attributes = MakeShared<FJsonObject>();
    Attributes->SetNumberField(TEXT("latitude"), Lat);
    Attributes->SetNumberField(TEXT("longitude"), Lon);
    Body->SetObjectField(TEXT("attributes"), Attributes);

    return Body;
}
