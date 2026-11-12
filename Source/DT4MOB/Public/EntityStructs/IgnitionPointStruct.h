/**
 * @file
 * @brief Data structs for wildfire ignition points ("fire:" things) and their cone and perimeter features.
 * @ingroup entitystructs
 */
#pragma once

#include "CoreMinimal.h"
#include "GeotileUtils.h"
#include "IgnitionPointStruct.generated.h"

/** @brief Position where the fire started. @ingroup entitystructs */
USTRUCT(BlueprintType)
struct DT4MOB_API FFireIgnitionPoint
{
    GENERATED_USTRUCT_BODY()

    /** @brief Latitude in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    double lat = 0.0;

    /** @brief Longitude in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    double lon = 0.0;
};

/** @brief Ditto attributes of an ignition point. @ingroup entitystructs */
USTRUCT(BlueprintType)
struct DT4MOB_API FIgnitionPointAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Where the fire started. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FFireIgnitionPoint fire_ignition;

    /** @brief Processing state reported by the backend. New points created in the viewer start as "new_ignition". Shown in the Info tab. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString state = TEXT("new_ignition");

    /** @brief GLB model URLs: index 0 = fire cone, index 1 = fire simulation mesh. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    TArray<FString> polygon;

    /** @brief UTC expiry time in ISO 8601 form without a trailing "Z". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString expiry_ts;

    /** @brief OSM quadkey at zoom-31, computed from fire_ignition coordinates on entity creation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    int64 geotile = 0;
};

// --- Cone feature ---
// features.cone.properties.perimeters is a single GeoJSON URL (cone horizon).

/** @brief Properties of the "cone" feature. @ingroup entitystructs */
USTRUCT(BlueprintType)
struct DT4MOB_API FFireConeFeatureProperties
{
    GENERATED_USTRUCT_BODY()

    /** @brief URL of the GeoJSON with the cone horizon perimeter. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString perimeters;
};

/** @brief The "cone" feature of an ignition point. @ingroup entitystructs */
USTRUCT(BlueprintType)
struct DT4MOB_API FFireConeFeature
{
    GENERATED_USTRUCT_BODY()

    /** @brief Feature properties. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FFireConeFeatureProperties properties;
};

// --- Perimeters feature ---
// features.perimeters.properties.perimeters is an array of GeoJSON URLs, one per time step.

/** @brief Properties of the "perimeters" feature. @ingroup entitystructs */
USTRUCT(BlueprintType)
struct DT4MOB_API FFirePerimeterFeatureProperties
{
    GENERATED_USTRUCT_BODY()

    /** @brief GeoJSON URLs of the simulated fire perimeter, one per simulation time step. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    TArray<FString> perimeters;
};

/** @brief The "perimeters" feature of an ignition point. @ingroup entitystructs */
USTRUCT(BlueprintType)
struct DT4MOB_API FFirePerimeterFeature
{
    GENERATED_USTRUCT_BODY()

    /** @brief Feature properties. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FFirePerimeterFeatureProperties properties;
};

// --- Features root ---

/** @brief Ditto features of an ignition point. @ingroup entitystructs */
USTRUCT(BlueprintType)
struct DT4MOB_API FIgnitionPointFeatures
{
    GENERATED_USTRUCT_BODY()

    /** @brief Cone horizon perimeter. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FFireConeFeature cone;

    /** @brief Simulated fire spread perimeters. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FFirePerimeterFeature perimeters;
};

// --- Root thing ---

/**
 * @brief Root of a wildfire ignition point thing (factory key "fire:", extension UIgnitionPointExtension).
 *
 * Unlike the other types, ignition points can be created from the viewer; MakeDefault() builds the
 * initial data for that.
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FIgnitionPointData
{
    GENERATED_USTRUCT_BODY()

    /** @brief Ditto thing id. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString thingId;

    /** @brief Ditto policy id that controls access to the thing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString policyId = TEXT("dt4mob:default");

    /** @brief Ditto attributes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FIgnitionPointAttributes attributes;

    /** @brief Ditto features. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FIgnitionPointFeatures features;

    /**
     * @brief Builds the data for a new ignition point at the given position.
     *
     * Sets the ignition coordinates, the geotile, and an expiry 24 hours from now.
     * @param Lat Latitude in decimal degrees.
     * @param Lon Longitude in decimal degrees.
     * @return The new instance.
     */
    static FIgnitionPointData MakeDefault(double Lat, double Lon)
    {
        FIgnitionPointData Data;
        Data.attributes.fire_ignition.lat = Lat;
        Data.attributes.fire_ignition.lon = Lon;
        Data.attributes.geotile = FGeotileUtils::LatLonToGeotile(Lat, Lon);

        // UTC expiry 24 h from now, formatted without trailing "Z" to match server format.
        const FDateTime Expiry = FDateTime::UtcNow() + FTimespan::FromHours(24);
        Data.attributes.expiry_ts = FString::Printf(
            TEXT("%04d-%02d-%02dT%02d:%02d:%02d.%03d000"),
            Expiry.GetYear(), Expiry.GetMonth(), Expiry.GetDay(),
            Expiry.GetHour(), Expiry.GetMinute(), Expiry.GetSecond(),
            Expiry.GetMillisecond());
        return Data;
    }
};
