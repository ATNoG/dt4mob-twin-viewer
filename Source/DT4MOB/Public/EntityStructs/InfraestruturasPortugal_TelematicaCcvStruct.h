/**
 * @file
 * @brief Ditto JSON structs for Infraestruturas de Portugal CCV telematics things
 *        (`telematica-ccv`).
 * @ingroup entitystructs
 */
#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_TelematicaCcvStruct.generated.h"

/**
 * @brief WGS-84 position of a CCV telematics site (`attributes.location`).
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtTelematicaCcvLocation
{
    GENERATED_USTRUCT_BODY()

    /** @brief WGS-84 latitude in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double latitude = 0.0;

    /** @brief WGS-84 longitude in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double longitude = 0.0;
};

/**
 * @brief Contents of the `attributes` object for a CCV telematics site.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtTelematicaCcvAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Record identifier of the asset in the IP source data set. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ID = 0;

    /** @brief WGS-84 position of the asset, used by ATempUIActor to place the actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtTelematicaCcvLocation location;

    /**
     * @brief Map tile quadkey that tile streaming filters on (`attributes/geotile` in Ditto
     * RQL).
     *
     * Static infrastructure imports encode it at zoom 31. A double cannot hold every
     * 62-bit zoom-31 key exactly, so do not use this copy for tile math.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double geotile = 0.0;

    /**
     * @brief Identifiers of the weather stations closest to the asset, as provided by the
     * backend.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> closest_meteo_stations;
};

/**
 * @brief Ditto thing for one CCV telematics site from the Infraestruturas de Portugal (IP)
 * `telematica-ccv` data set.
 *
 * RegisterAllEntityTypes() maps thingIds containing `telematica-ccv` to this struct,
 * with UTelematicaCcvExtension supplying the type-specific behavior. FJsonObjectConverter
 * fills it from the thing JSON, so field names match the JSON keys.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtTelematicaCcvData
{
    GENERATED_USTRUCT_BODY()

    /**
     * @brief Ditto thing ID; UDT4MOBEntityFactory picks this struct for IDs containing
     * `telematica-ccv`.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    /** @brief ID of the Ditto policy that controls access to the thing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    /** @brief Asset data from the thing's `attributes` object. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtTelematicaCcvAttributes attributes;
};
