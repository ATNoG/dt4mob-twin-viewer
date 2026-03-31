#pragma once

#include "CoreMinimal.h"
#include "MeteorologyStruct.generated.h"

/**
 * @brief Raw sensor readings from a meteorological station at a single point in time.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FMeteorologyProperties
{

    GENERATED_USTRUCT_BODY()

    /** @brief Wind intensity in m/s. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteorology")
    float wind_intensity = 0.0f;

    /** @brief Ambient temperature in degrees Celsius. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteorology")
    float temperature = 0.0f;

    /** @brief Solar radiation in W/m². */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteorology")
    float radiation = 0.0f;

    /** @brief Wind direction in degrees (meteorological convention: 0=N, 90=E). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteorology")
    int wind_direction = 0;

    /** @brief Accumulated precipitation in mm. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteorology")
    float accumulated_precipitation = 0.0f;

    /** @brief Atmospheric pressure in hPa. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteorology")
    float pressure = 0.0f;

    /** @brief Relative humidity percentage (0–100). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteorology")
    float humidity = 0.0f;

    /** @brief ISO-8601 timestamp of the measurement. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteorology")
    FString time;

    /** @brief Returns a human-readable summary of all meteorology properties. */
    FString toString() const
    {
        return FString::Printf(TEXT("Time: %s\nTemp: %.2f\nHumidity: %.2f\nPressure: %.2f\nWind Intensity: %.2f\nWind Direction: %d\nRadiation: %.2f\nAccumulated Precipitation: %.2f"),
                               *time, temperature, humidity, pressure, wind_intensity, wind_direction, radiation, accumulated_precipitation);
    }
};

/**
 * @brief Container for a single meteorology feature's property set.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FMeteorologyStruct
{
    GENERATED_USTRUCT_BODY()

    /** @brief The meteorology sensor readings. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteorology")
    FMeteorologyProperties properties;

    /** @brief Returns a human-readable summary. */
    FString toString() const
    {
        return FString::Printf(TEXT("Properties:\n\t%s"), *properties.toString());
    }
};

/**
 * @brief Ditto features block for a meteorology thing, containing the meteorology feature.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FMeteorologyFeature
{
    GENERATED_USTRUCT_BODY()

    /** @brief The meteorology feature data. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteorology")
    FMeteorologyStruct meteorology;

    /** @brief Returns a human-readable summary. */
    FString toString() const
    {
        return FString::Printf(TEXT("Meteorology:\n\t%s"), *meteorology.toString());
    }
};

/**
 * @brief Geographic location sub-struct specific to meteorology stations.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FMeteoLocationData
{
    GENERATED_USTRUCT_BODY()

    /** @brief Geographic longitude in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
    double longitude = 0.0;

    /** @brief Geographic latitude in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
    double latitude = 0.0;
};

/**
 * @brief Attributes block for a meteorology station Ditto thing.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FMeteoAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Numeric identifier of the meteorology station. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
    int32 id = 0;

    /** @brief Geographic position of the station. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
    FMeteoLocationData location;

    /** @brief Human-readable name of the station location. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
    FString location_name;

    /** @brief Returns a human-readable summary of the attributes. */
    FString toString() const
    {
        return FString::Printf(TEXT("ID: %d\nLocation: %s\nLatitude: %.6f\nLongitude: %.6f"),
                               id, *location_name, location.latitude, location.longitude);
    }
};

/**
 * @brief Root Ditto thing struct for a meteorology station entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FMeteorologyData
{
    GENERATED_USTRUCT_BODY()

    /** @brief Ditto thing identifier (e.g. "meteo:station-1"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteorology")
    FString thingId;

    /** @brief Ditto policy identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteorology")
    FString policyId;

    /** @brief Static attributes of the meteorology station (ID, location, name). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteorology")
    FMeteoAttributes attributes;

    /** @brief Live feature data containing the latest sensor readings. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Meteorology")
    FMeteorologyFeature features;

    /** @brief Returns a human-readable summary of the full thing data. */
    FString toString() const
    {
        return FString::Printf(TEXT("ThingId: %s\nPolicyId: %s\nAttributes:\n\t%s\nFeatures:\n\t%s"),
                               *thingId, *policyId, *attributes.toString(), *features.toString());
    }
};
