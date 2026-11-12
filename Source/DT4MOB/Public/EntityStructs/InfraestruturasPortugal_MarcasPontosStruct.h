/**
 * @file
 * @brief Ditto JSON structs for Infraestruturas de Portugal point road marking things
 *        (`marcas-pontos`).
 * @ingroup entitystructs
 */
#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_MarcasPontosStruct.generated.h"

/**
 * @brief WGS-84 position of a point road marking (`attributes.location`).
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtMarcasPontosLocation
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
 * @brief Contents of the `attributes` object for a point road marking.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtMarcasPontosAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Record identifier of the asset in the IP source data set. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ID = 0;

    /**
     * @brief Road marking type code from the data set.
     *
     * Shown as "Code" in the info tab by default.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Code;

    /** @brief WGS-84 position of the asset, used by ATempUIActor to place the actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtMarcasPontosLocation location;

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
 * @brief Ditto thing for one point road marking from the Infraestruturas de Portugal (IP)
 * `marcas-pontos` data set.
 *
 * RegisterAllEntityTypes() maps thingIds containing `marcas-pontos` to this struct,
 * with UMarcasPontosExtension supplying the type-specific behavior. FJsonObjectConverter
 * fills it from the thing JSON, so field names match the JSON keys.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtMarcasPontosData
{
    GENERATED_USTRUCT_BODY()

    /**
     * @brief Ditto thing ID; UDT4MOBEntityFactory picks this struct for IDs containing
     * `marcas-pontos`.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    /** @brief ID of the Ditto policy that controls access to the thing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    /** @brief Asset data from the thing's `attributes` object. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtMarcasPontosAttributes attributes;
};
