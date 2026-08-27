// Fill out your copyright notice in the Description page of Project Settings.

#include "EntityDependencies/Sinalizacao/SinalizacaoExtension.h"

TArray<FInfoField> USinalizacaoExtension::GetDefaultInfoFields() const
{
    return {
        { "Code",      "attributes.Code"                },
        { "Latitude",  "attributes.location.latitude"  },
        { "Longitude", "attributes.location.longitude" },
    };
}

FLinearColor USinalizacaoExtension::GetBadgeColor() const
{
    return FLinearColor(0.859f, 0.659f, 0.337f); // amber
}

FString USinalizacaoExtension::GetBadgeLabel(const FString& TypeKey) const
{
    return TEXT("SIGN");
}

FString USinalizacaoExtension::GetFullFetchFilter(const FString& TypeKey) const
{
    return FString::Printf(TEXT("like(thingId,\"*%s*\")"), *TypeKey);
}
