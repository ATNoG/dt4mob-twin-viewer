/**
 * @file
 * @brief Ditto JSON structs for Infraestruturas de Portugal linear drainage things
 *        (`drenagem-linear`).
 * @ingroup entitystructs
 */
#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_DrenagemLinearStruct.generated.h"

/**
 * @brief One vertex of a linear drainage element's `attributes.geometry`, in WGS-84.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtDrenagemLinearPoint
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
 * @brief Contents of the `attributes` object for a linear drainage element.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtDrenagemLinearAttributes
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
     * @brief Ordered vertices of the asset path or outline.
     *
     * ATempUIActor places the actor at the mean of these points, and
     * UEntityGeometryMeshComponent builds its mesh from them when mesh generation is
     * enabled.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FInfPtDrenagemLinearPoint> geometry;

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
 * @brief Ditto thing for one linear drainage element from the Infraestruturas de Portugal (IP)
 * `drenagem-linear` data set.
 *
 * RegisterAllEntityTypes() maps thingIds containing `drenagem-linear` to this struct,
 * with UDrenagemLinearExtension supplying the type-specific behavior. FJsonObjectConverter
 * fills it from the thing JSON, so field names match the JSON keys.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtDrenagemLinearData
{
    GENERATED_USTRUCT_BODY()

    /**
     * @brief Ditto thing ID; UDT4MOBEntityFactory picks this struct for IDs containing
     * `drenagem-linear`.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    /** @brief ID of the Ditto policy that controls access to the thing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    /** @brief Asset data from the thing's `attributes` object. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtDrenagemLinearAttributes attributes;
};
