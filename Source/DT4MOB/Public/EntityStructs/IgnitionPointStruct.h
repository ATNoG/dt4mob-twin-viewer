#pragma once

#include "CoreMinimal.h"
#include "GeotileUtils.h"
#include "IgnitionPointStruct.generated.h"

USTRUCT(BlueprintType)
struct DT4MOB_API FFireIgnitionPoint
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    double lat = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    double lon = 0.0;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FIgnitionPointAttributes
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FFireIgnitionPoint fire_ignition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString state = TEXT("new_ignition");

    /** GLB model URLs — index 0 = fire cone, index 1 = fire simulation mesh. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    TArray<FString> polygon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString expiry_ts;

    /** OSM quadkey at zoom-31, computed from fire_ignition coordinates on entity creation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    int64 geotile = 0;
};

// ── Cone feature ──────────────────────────────────────────────────────────────
// features.cone.properties.perimeters is a single GeoJSON URL (cone horizon).

USTRUCT(BlueprintType)
struct DT4MOB_API FFireConeFeatureProperties
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString perimeters;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FFireConeFeature
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FFireConeFeatureProperties properties;
};

// ── Perimeters feature ────────────────────────────────────────────────────────
// features.perimeters.properties.perimeters is an array of GeoJSON URLs, one per time step.

USTRUCT(BlueprintType)
struct DT4MOB_API FFirePerimeterFeatureProperties
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    TArray<FString> perimeters;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FFirePerimeterFeature
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FFirePerimeterFeatureProperties properties;
};

// ── Features root ─────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct DT4MOB_API FIgnitionPointFeatures
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FFireConeFeature cone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FFirePerimeterFeature perimeters;
};

// ── Root thing ────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct DT4MOB_API FIgnitionPointData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString thingId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString policyId = TEXT("dt4mob:default");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FIgnitionPointAttributes attributes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FIgnitionPointFeatures features;

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
