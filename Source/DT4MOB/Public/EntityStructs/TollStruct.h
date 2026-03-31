#pragma once

#include "CoreMinimal.h"
#include "TollCameraStruct.h"
#include "TollStruct.generated.h"

/**
 * @brief Geographic coordinates of a toll plaza.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTollCoordinates
{
    GENERATED_USTRUCT_BODY()

    /** @brief Geographic latitude in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolls")
    double latitude = 0.0;

    /** @brief Geographic longitude in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolls")
    double longitude = 0.0;
};

/**
 * @brief Static attributes of a toll plaza entity.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTollAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Numeric identifier of the toll plaza. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolls")
    int32 id = 0;

    /** @brief Human-readable name of the toll plaza. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolls")
    FString name;

    /** @brief Geographic position of the toll plaza. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolls")
    FTollCoordinates coordinates;
};

/**
 * @brief A single vehicle detection entry as stored inside the toll feature maps.
 *
 * Matches the per-object entries under features.camera-ORT and features.lidar-outsight.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTollVehicleDetection
{
    GENERATED_USTRUCT_BODY()

    /** @brief Track/object identifier string assigned by the sensor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolls")
    FString id;

    /** @brief ISO-8601 timestamp of the detection event. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolls")
    FString ts;

    /** @brief Detailed detection payload (speed, position, dimensions, etc.). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolls")
    FTollCameraDetectionData extra;
};

/**
 * @brief Ditto features block for a toll plaza entity.
 *
 * Stores live vehicle detection maps from the ORT camera and the LiDAR sensor.
 * Map keys are detection entry identifiers (e.g. "1", "2").
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTollFeatures
{
    GENERATED_USTRUCT_BODY()

    /** @brief Vehicle detections from the "camera-ORT" feature; keyed by entry identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolls")
    TMap<FString, FTollVehicleDetection> cameraORT;

    /** @brief Vehicle detections from the "lidar-outsight" feature; keyed by entry identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolls")
    TMap<FString, FTollVehicleDetection> lidarOutsight;
};

/**
 * @brief Root Ditto thing struct for a toll plaza entity.
 *
 * Aggregates static location attributes with live vehicle detection data from
 * ORT cameras and LiDAR sensors. Maps directly from Ditto JSON via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTollData
{
    GENERATED_USTRUCT_BODY()

    /** @brief Ditto thing identifier (e.g. "tolls:toll-1"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolls")
    FString thingId;

    /** @brief Ditto policy identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolls")
    FString policyId;

    /** @brief Static attributes (ID, name, coordinates) of the toll plaza. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolls")
    FTollAttributes attributes;

    /** @brief Live vehicle detection data from ORT camera and LiDAR sensor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tolls")
    FTollFeatures features;

    /** @brief Returns a human-readable summary of the toll plaza entity. */
    FString toString() const
    {
        return FString::Printf(TEXT("Toll - thingId: %s, name: %s, lat: %.6f, lon: %.6f, cameraORT: %d vehicles, lidar: %d vehicles"),
                               *thingId,
                               *attributes.name,
                               attributes.coordinates.latitude,
                               attributes.coordinates.longitude,
                               features.cameraORT.Num(),
                               features.lidarOutsight.Num());
    }
};
