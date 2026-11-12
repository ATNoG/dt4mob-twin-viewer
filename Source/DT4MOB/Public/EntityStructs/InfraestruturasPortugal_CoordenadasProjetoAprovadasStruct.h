/**
 * @file
 * @brief Ditto JSON structs for Infraestruturas de Portugal approved project coordinate things
 *        (`coordenadas-de-projeto-aprovadas-ip`).
 * @ingroup entitystructs
 */
#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_CoordenadasProjetoAprovadasStruct.generated.h"

/**
 * @brief WGS-84 position of an approved project coordinate point (`attributes.location`).
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtCoordenadasProjetoAprovadasLocation
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
 * @brief Contents of the `attributes` object for an approved project coordinate point.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtCoordenadasProjetoAprovadasAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Raw `Field1` value from the source table (column meaning not documented). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Field1;

    /**
     * @brief Raw X coordinate from the source table, in a coordinate system not identified
     * here.
     *
     * The actor is placed from `location`, not from X, Y and Z.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double X = 0.0;

    /** @brief Raw Y coordinate from the source table, in the same coordinate system as X. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double Y = 0.0;

    /**
     * @brief Raw Z coordinate from the source table (units unspecified).
     *
     * Not used for actor placement.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double Z = 0.0;

    /** @brief WGS-84 position of the asset, used by ATempUIActor to place the actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtCoordenadasProjetoAprovadasLocation location;

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
 * @brief Ditto thing for one approved project coordinate point from the Infraestruturas de Portugal (IP)
 * `coordenadas-de-projeto-aprovadas-ip` data set.
 *
 * RegisterAllEntityTypes() maps thingIds containing `coordenadas-de-projeto-aprovadas-ip` to this struct,
 * with UCoordenadasProjetoAprovadasExtension supplying the type-specific behavior. FJsonObjectConverter
 * fills it from the thing JSON, so field names match the JSON keys.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtCoordenadasProjetoAprovadasData
{
    GENERATED_USTRUCT_BODY()

    /**
     * @brief Ditto thing ID; UDT4MOBEntityFactory picks this struct for IDs containing
     * `coordenadas-de-projeto-aprovadas-ip`.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    /** @brief ID of the Ditto policy that controls access to the thing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    /** @brief Asset data from the thing's `attributes` object. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtCoordenadasProjetoAprovadasAttributes attributes;
};
