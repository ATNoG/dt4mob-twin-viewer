#pragma once

#include "CoreMinimal.h"
#include "CarStruct.generated.h"

/**
 * @brief Real-time kinematic properties of a vehicle entity (TRACI/simulation source).
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FCarProperties
{
    GENERATED_USTRUCT_BODY()

    /** @brief Longitudinal acceleration in m/s². */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    double accel = 0.0;

    /** @brief Current speed in m/s. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    double speed = 0.0;

    /** @brief Heading angle in degrees (0 = north, clockwise). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    double angle = 0.0;

    /** @brief Geographic latitude in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    double latitude = 0.0;

    /** @brief Geographic longitude in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    double longitude = 0.0;
};

/**
 * @brief Wraps the car's real-time property set as a Ditto feature state.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FCarState
{
    GENERATED_USTRUCT_BODY()

    /** @brief Live kinematic properties of the vehicle. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    FCarProperties properties;
};

/**
 * @brief Ditto features block for a vehicle entity.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FCarFeatures
{
    GENERATED_USTRUCT_BODY()

    /** @brief Current vehicle state (position, speed, angle). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    FCarState state;
};

/**
 * @brief Static attributes of a vehicle entity.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FCarAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Vehicle length in metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    int length = 0;

    /** @brief Time-to-live in seconds; entity should be removed when expired. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    int time_to_live = 0;
};

/**
 * @brief Root Ditto thing struct for a vehicle (TRACI) entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FCarData
{
    GENERATED_USTRUCT_BODY()

    /** @brief Ditto thing identifier (e.g. "traci:vehicle-42"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    FString thingId;

    /** @brief Ditto policy identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    FString policyId;

    /** @brief Static vehicle attributes (length, TTL). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    FCarAttributes attributes;

    /** @brief Live features containing the vehicle's current state. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    FCarFeatures features;

    /** @brief Returns a human-readable summary of the vehicle entity. */
    FString ToString() const
    {
        return FString::Printf(TEXT("Thing ID: %s\nSpeed: %f\nAcceleration: %f\nAngle: %f\nLatitude: %f\nLongitude: %f"),
                               *thingId, features.state.properties.speed, features.state.properties.accel, features.state.properties.angle,
                               features.state.properties.latitude, features.state.properties.longitude);
    }
};
