/**
 * @file
 * @brief Ditto JSON structs for Infraestruturas de Portugal video surveillance things
 *        (`telematica-sistemas-videovigilancia`).
 * @ingroup entitystructs
 */
#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_TelematicaVideovigilanciaStruct.generated.h"

/**
 * @brief WGS-84 position of a video surveillance installation (`attributes.location`).
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtTelematicaVideovigilanciaLocation
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
 * @brief Contents of the `attributes` object for a video surveillance installation.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtTelematicaVideovigilanciaAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Record identifier of the asset in the IP source data set. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ID = 0;

    /** @brief WGS-84 position of the asset, used by ATempUIActor to place the actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtTelematicaVideovigilanciaLocation location;

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
 * @brief Ditto thing for one video surveillance installation from the Infraestruturas de Portugal (IP)
 * `telematica-sistemas-videovigilancia` data set.
 *
 * RegisterAllEntityTypes() maps thingIds containing `telematica-sistemas-videovigilancia` to this struct,
 * with UTelematicaVideovigilanciaExtension supplying the type-specific behavior. FJsonObjectConverter
 * fills it from the thing JSON, so field names match the JSON keys.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtTelematicaVideovigilanciaData
{
    GENERATED_USTRUCT_BODY()

    /**
     * @brief Ditto thing ID; UDT4MOBEntityFactory picks this struct for IDs containing
     * `telematica-sistemas-videovigilancia`.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    /** @brief ID of the Ditto policy that controls access to the thing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    /** @brief Asset data from the thing's `attributes` object. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtTelematicaVideovigilanciaAttributes attributes;
};
