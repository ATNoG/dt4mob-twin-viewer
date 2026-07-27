// Fill out your copyright notice in the Description page of Project Settings.

#include "EntityDependencies/Meteo/MeteoExtension.h"

TArray<FInfoField> UMeteoExtension::GetDefaultInfoFields() const
{
    return {
        { "Station",       "attributes.location_name"                                  },
        { "Latitude",      "attributes.location.latitude"                              },
        { "Longitude",     "attributes.location.longitude"                             },
        { "Temperature",   "features.meteorology.properties.temperature"               },
        { "Humidity",      "features.meteorology.properties.humidity"                  },
        { "Wind Speed",    "features.meteorology.properties.wind_intensity"            },
        { "Wind Dir.",     "features.meteorology.properties.wind_direction"            },
        { "Pressure",      "features.meteorology.properties.pressure"                  },
        { "Precipitation", "features.meteorology.properties.accumulated_precipitation" },
        { "Last Reading",  "features.meteorology.properties.time"                      },
    };
}

FLinearColor UMeteoExtension::GetBadgeColor() const
{
    return FLinearColor(0.800f, 0.533f, 1.000f); // purple
}

FString UMeteoExtension::GetBadgeLabel(const FString& TypeKey) const
{
    return TEXT("METEO");
}
