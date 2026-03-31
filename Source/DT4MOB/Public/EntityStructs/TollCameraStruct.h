#pragma once

#include "CoreMinimal.h"
#include "TollCameraStruct.generated.h"

/**
 * @brief Sensor-frame 2D coordinates of a detected object (relative to the camera sensor origin).
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTollCameraLocalCoordinates
{
    GENERATED_USTRUCT_BODY()

    /** @brief X coordinate in the sensor local frame (metres). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    double x = 0.0;

    /** @brief Y coordinate in the sensor local frame (metres). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    double y = 0.0;
};

/**
 * @brief WGS-84 geographic coordinates of a detected vehicle.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTollCameraAbsoluteCoordinates
{
    GENERATED_USTRUCT_BODY()

    /** @brief Geographic latitude in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    double latitude = 0.0;

    /** @brief Geographic longitude in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    double longitude = 0.0;
};

/**
 * @brief Bounding-box dimensions of a detected vehicle.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTollCameraDimensions
{
    GENERATED_USTRUCT_BODY()

    /** @brief Vehicle bounding-box width in metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    double width = 0.0;

    /** @brief Vehicle bounding-box length in metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    double length = 0.0;

    /** @brief Vehicle bounding-box height in metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    double height = 0.0;
};

/**
 * @brief A single vehicle detection event from the toll camera or LiDAR sensor.
 *
 * Populated from the "extra" sub-object inside a Ditto feature property entry.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTollCameraDetectionData
{
    GENERATED_USTRUCT_BODY()

    /** @brief ISO-8601 timestamp of the measurement. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    FString timeOfMeasurement;

    /** @brief Vehicle speed in km/h at time of measurement. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    double speedKmh = 0.0;

    /** @brief Position in the sensor local coordinate frame. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    FTollCameraLocalCoordinates localCoordinates;

    /** @brief Geographic (WGS-84) position of the detected vehicle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    FTollCameraAbsoluteCoordinates absoluteCoordinates;

    /** @brief Bounding-box dimensions of the detected vehicle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    FTollCameraDimensions dimensions;

    /** @brief Unique object/track identifier assigned by the sensor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    int32 objectID = 0;

    /** @brief Returns a human-readable summary of the detection data. */
    FString toString() const
    {
        return FString::Printf(TEXT("objectID: %d, speed: %.2f km/h, lat: %.6f, lon: %.6f, dims: %.2fx%.2fx%.2f, time: %s"),
                               objectID, speedKmh,
                               absoluteCoordinates.latitude, absoluteCoordinates.longitude,
                               dimensions.width, dimensions.length, dimensions.height,
                               *timeOfMeasurement);
    }
};

/**
 * @brief Wraps a FTollCameraDetectionData as a Ditto feature state.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTollCameraState
{
    GENERATED_USTRUCT_BODY()

    /** @brief Latest detection data reported by the sensor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    FTollCameraDetectionData properties;
};

/**
 * @brief Ditto features block for a toll camera entity.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTollCameraFeatures
{
    GENERATED_USTRUCT_BODY()

    /** @brief Current camera state containing the latest detection. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    FTollCameraState State;
};

/**
 * @brief Static attributes of a toll camera sensor entity.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTollCameraAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Numeric identifier of the camera. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    int32 id = 0;

    /** @brief ISO-8601 expiry timestamp; the entity should be removed after this time. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    FString expiry_ts;
};

/**
 * @brief Root Ditto thing struct for a toll camera (ORT or LiDAR) entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTollCameraData
{
    GENERATED_USTRUCT_BODY()

    /** @brief Ditto thing identifier (e.g. "tolls:camera-ORT-1"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    FString thingId;

    /** @brief Ditto policy identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    FString policyId;

    /** @brief Static sensor attributes (id, expiry). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    FTollCameraAttributes attributes;

    /** @brief Live features containing the latest detection state. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TollCamera")
    FTollCameraFeatures features;

    /** @brief Returns a human-readable summary of the toll camera entity. */
    FString toString() const
    {
        return FString::Printf(TEXT("TollCamera - thingId: %s, id: %d, expiry: %s, detection: [%s]"),
                               *thingId,
                               attributes.id,
                               *attributes.expiry_ts,
                               *features.State.properties.toString());
    }
};
