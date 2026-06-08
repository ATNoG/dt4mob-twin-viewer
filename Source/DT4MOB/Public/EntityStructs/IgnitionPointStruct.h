#pragma once

#include "CoreMinimal.h"
#include "IgnitionPointStruct.generated.h"

USTRUCT(BlueprintType)
struct DT4MOB_API FFireIgnitionPoint
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    double lat = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    double lon = 0.0;

    FString toString() const
    {
        return FString::Printf(TEXT("FireIgnitionPoint - Lat: %.6f, Lon: %.6f"), lat, lon);
    }
};

USTRUCT(BlueprintType)
struct DT4MOB_API FIgnitionPointAttributes
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FFireIgnitionPoint fire_ignition = FFireIgnitionPoint();

    /** @brief Simulation status: "new_ignition" | "burning" | "extinguished". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString state = TEXT("new_ignition");
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString polygon = TEXT("");
    
    FString toString() const
    {
        return FString::Printf(TEXT("IgnitionPointAttributes - Lat: %.6f, Lon: %.6f, State: %s"),
                               fire_ignition.lat, fire_ignition.lon, *state);
    }
};

/** @brief A single lat/lon vertex used in fire simulation geometry. */
USTRUCT(BlueprintType)
struct DT4MOB_API FFireGeoPoint
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    double lat = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    double lon = 0.0;
};

/**
 * @brief One simulation horizon slice — a polygon at a given time horizon (30/60/90/120 min).
 * Populated by the simulation after a successful run; empty on initial placement.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FFireShape
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    TArray<FFireGeoPoint> points;

    /** @brief Time horizon in minutes. Expected values: 30, 60, 90, 120. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    int32 horizonte_min = 0;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FFireFeatureProperties
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    TArray<FFireShape> perimeters;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FFireFeature
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FFireFeatureProperties properties;
};

/**
 * @brief Ditto features populated after a successful simulation run.
 *
 * cone     — 4 slices (30/60/90/120 min), each a small polygon near the ignition point.
 * perimeters — 4 slices with the same horizonte_min values but many more boundary points.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FIgnitionPointFeatures
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FFireFeature cone;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FFireFeature perimeters;
};

/**
 * @brief Root Ditto thing struct for a user-placed ignition point.
 *
 * thingId format: "fire:<guid>"
 * State flows: new_ignition → burning → extinguished as the fire simulation progresses.
 * features (cone, perimeters) are absent on creation and patched in by the simulation service.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FIgnitionPointData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString thingId = FString();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString policyId = TEXT("fire:default");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FIgnitionPointAttributes attributes;

    /** @brief Populated after a successful simulation run; empty on initial placement. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FIgnitionPointFeatures features;

    /** @brief Display name shown in the entity type dropdown. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    FString DisplayName = TEXT("Ignition Point");

    /** @brief If true, show a warning that this entity type has no server-side handling. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IgnitionPoint")
    bool bNoServerHandling = false;

    static FIgnitionPointData MakeDefault(double Lat, double Lon)
    {
        FIgnitionPointData Data;
        Data.attributes.fire_ignition.lat = Lat;
        Data.attributes.fire_ignition.lon = Lon;
        return Data;
    }

    FString toString() const
    {
        return FString::Printf(TEXT("IgnitionPointData - ThingId: %s, PolicyId: %s, Attributes: %s"),
                               *thingId, *policyId, *attributes.toString());
    }
};
