// Fill out your copyright notice in the Description page of Project Settings.

#include "EntityDependencies/GeoInstrument/GeoInstrumentExtension.h"

TArray<FInfoField> UGeoInstrumentExtension::GetDefaultInfoFields() const
{
    return {
        { "Matricula",  "attributes.matricula"             },
        { "Type",       "attributes.tipoInstrumento.name"  },
        { "Latitude",   "attributes.coordinates.latitude"  },
        { "Longitude",  "attributes.coordinates.longitude" },
    };
}

FLinearColor UGeoInstrumentExtension::GetBadgeColor() const
{
    return FLinearColor(0.557f, 0.882f, 0.533f); // green
}

FString UGeoInstrumentExtension::GetBadgeLabel(const FString& TypeKey) const
{
    return TEXT("INSTR");
}
