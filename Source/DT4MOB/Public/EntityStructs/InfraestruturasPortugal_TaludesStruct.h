/**
 * @file
 * @brief Ditto JSON structs for Infraestruturas de Portugal slope and embankment things
 *        (`taludes`).
 * @ingroup entitystructs
 */
#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_TaludesStruct.generated.h"

/**
 * @brief One vertex of a slope or embankment's `attributes.geometry`, in WGS-84.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtTaludesPoint
{
    GENERATED_USTRUCT_BODY()

    /** @brief WGS-84 latitude of the vertex in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double latitude = 0.0;

    /** @brief WGS-84 longitude of the vertex in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double longitude = 0.0;
};

/**
 * @brief Contents of the `attributes` object for a slope or embankment.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtTaludesAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Record identifier of the asset in the IP source data set. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ID = 0;

    /**
     * @brief Length of the asset geometry from the source GIS export (units unspecified).
     *
     * Shown as "Length" in the info tab by default.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double shape_Length = 0.0;

    /**
     * @brief Area of the asset geometry from the source GIS export (units unspecified).
     *
     * Shown as "Area" in the info tab by default. UEntityGeometryMeshComponent treats a
     * non-zero value as a closed shape and fills it.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double shape_Area = 0.0;

    /**
     * @brief Ordered vertices of the asset outline.
     *
     * ATempUIActor places the actor at the mean of these points, and
     * UEntityGeometryMeshComponent builds its mesh from them when mesh generation is
     * enabled.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FInfPtTaludesPoint> geometry;

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
 * @brief Ditto thing for one slope or embankment from the Infraestruturas de Portugal (IP)
 * `taludes` data set.
 *
 * RegisterAllEntityTypes() maps thingIds containing `taludes` to this struct,
 * with UTaludesExtension supplying the type-specific behavior. FJsonObjectConverter
 * fills it from the thing JSON, so field names match the JSON keys.
 *
 * Not the same schema as FTaludeData, which mirrors the older `muro-talude`
 * things.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtTaludesData
{
    GENERATED_USTRUCT_BODY()

    /**
     * @brief Ditto thing ID; UDT4MOBEntityFactory picks this struct for IDs containing
     * `taludes`.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    /** @brief ID of the Ditto policy that controls access to the thing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    /** @brief Asset data from the thing's `attributes` object. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtTaludesAttributes attributes;
};
