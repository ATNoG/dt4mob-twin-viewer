/**
 * @file
 * @brief Schema of "traci" vehicle things published by the traffic simulation.
 * @ingroup entitystructs
 */
#pragma once

#include "CoreMinimal.h"
#include "CarStruct.generated.h"

/**
 * @brief Live kinematic state of a simulated vehicle (`features.state.properties`).
 *
 * @note UVehicleExtension turns off the struct resync on live updates, so after spawn this
 *       struct keeps its initial values. ATempUIActor and the info tab read the live values
 *       from the raw JSON instead.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FCarProperties
{
    GENERATED_USTRUCT_BODY()

    /** @brief Acceleration in m/s^2. Shown in the info tab; the actor's motion does not use it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    double accel = 0.0;

    /**
     * @brief Speed in m/s, shown in the info tab. Motion interpolation is paced by the
     *        observed update interval, not by this value.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    double speed = 0.0;

    /** @brief Heading in degrees (0 = north, 90 = east, clockwise). The actor faces this way. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    double angle = 0.0;

    /** @brief Latitude in decimal degrees (WGS-84). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    double latitude = 0.0;

    /** @brief Longitude in decimal degrees (WGS-84). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    double longitude = 0.0;
};

/**
 * @brief The `state` feature of a vehicle: a Ditto feature whose properties hold the live
 *        kinematics.
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FCarState
{
    GENERATED_USTRUCT_BODY()

    /** @brief Current position, speed, heading and acceleration. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    FCarProperties properties;
};

/**
 * @brief The `features` block of a vehicle thing.
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FCarFeatures
{
    GENERATED_USTRUCT_BODY()

    /** @brief Live vehicle state, updated by the simulation on every step. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    FCarState state;
};

/**
 * @brief The `attributes` block of a vehicle thing.
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FCarAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Vehicle length in metres. ATempUIActor scales the placeholder mesh to it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    int length = 0;

    /**
     * @brief Time-to-live value from the simulation, shown as "TTL" in the info tab.
     *
     * The viewer does not enforce it. Vehicles are removed when they stop sending updates
     * for 60 seconds (see UVehicleExtension::LiveStalenessTimeoutSeconds()).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    int time_to_live = 0;
};

/**
 * @brief Root schema of a simulated vehicle thing, registered under the "traci" type key.
 *
 * FJsonObjectConverter fills it from the thing JSON when the actor is spawned.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FCarData
{
    GENERATED_USTRUCT_BODY()

    /** @brief Ditto thing ID (contains "traci"). ATempUIActor copies it into its ThingId. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    FString thingId;

    /** @brief ID of the Ditto policy that controls access to the thing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    FString policyId;

    /** @brief Static vehicle attributes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    FCarAttributes attributes;

    /** @brief Live features holding the vehicle state. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    FCarFeatures features;

    /**
     * @brief Label for this type. Not read by the viewer: the entity type dropdown takes its
     *        label from UDT4MOBEntityFactory::RegisterType() (see EntityTypeRegistrations.cpp).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    FString DisplayName = TEXT("Vehicle");

    /**
     * @brief Not read by the viewer. The "no server-side handling" warning comes from the flag
     *        passed to UDT4MOBEntityFactory::RegisterType().
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Car")
    bool bNoServerHandling = true;

    /**
     * @brief Builds a vehicle at the given position with every other field at its default.
     * @param Lat  Latitude in decimal degrees (WGS-84).
     * @param Lon  Longitude in decimal degrees (WGS-84).
     * @return The new instance.
     */
    static FCarData MakeDefault(double Lat, double Lon)
    {
        FCarData Data;
        Data.features.state.properties.latitude = Lat;
        Data.features.state.properties.longitude = Lon;
        return Data;
    }

    /**
     * @brief Formats the thing ID and live state for logging.
     * @return Multi-line text with ID, speed, acceleration, angle, latitude and longitude.
     */
    FString ToString() const
    {
        return FString::Printf(TEXT("Thing ID: %s\nSpeed: %f\nAcceleration: %f\nAngle: %f\nLatitude: %f\nLongitude: %f"),
                               *thingId, features.state.properties.speed, features.state.properties.accel, features.state.properties.angle,
                               features.state.properties.latitude, features.state.properties.longitude);
    }
};
