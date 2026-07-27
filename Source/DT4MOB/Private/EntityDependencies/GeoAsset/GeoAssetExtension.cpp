// Fill out your copyright notice in the Description page of Project Settings.

#include "EntityDependencies/GeoAsset/GeoAssetExtension.h"

TArray<FInfoField> UGeoAssetExtension::GetDefaultInfoFields() const
{
    return {
        { "Matricula",  "attributes.matricula"           },
        { "Location",   "attributes.localizacao"         },
        { "Latitude",   "attributes.latitude"            },
        { "Longitude",  "attributes.longitude"           },
        { "Condition",  "attributes.indiceCondicaoAtual" },
        { "Height",     "attributes.altura"              },
        { "Length",     "attributes.extensao"            },
    };
}

FLinearColor UGeoAssetExtension::GetBadgeColor() const
{
    return FLinearColor(0.557f, 0.882f, 0.533f); // green
}

FString UGeoAssetExtension::GetBadgeLabel(const FString& TypeKey) const
{
    return TEXT("GEO");
}
