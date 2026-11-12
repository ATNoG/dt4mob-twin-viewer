/**
 * @file
 * @brief Ditto JSON structs for Infraestruturas de Portugal route information line things
 *        (`info-route-ln`).
 * @ingroup entitystructs
 */
#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_InfoRouteLnStruct.generated.h"

/**
 * @brief One vertex of a road route segment's `attributes.geometry`, in WGS-84.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtInfoRouteLnPoint
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
 * @brief Contents of the `attributes` object for a road route segment.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtInfoRouteLnAttributes
{
    GENERATED_USTRUCT_BODY()

    /**
     * @brief Name of the route.
     *
     * Shown as "Route" in the info tab by default.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString route_name;

    /**
     * @brief Road number of the route.
     *
     * Shown as "Road No." in the info tab by default.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString roadnumber;

    /**
     * @brief Kilometer point where the segment starts.
     *
     * Shown as "Km Start" in the info tab by default.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double kmi = 0.0;

    /**
     * @brief Kilometer point where the segment ends.
     *
     * Shown as "Km End" in the info tab by default.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double kmf = 0.0;

    /**
     * @brief Raw `FIRST_POINT` number for the segment's first point, as exported by
     * the source layer.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double FIRST_POINT = 0.0;

    /**
     * @brief Raw `LAST_POINT` number for the segment's last point, as exported by the
     * source layer.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double LAST_POINT = 0.0;

    /**
     * @brief Length of the asset geometry from the source GIS export (units unspecified).
     *
     * Shown as "Length" in the info tab by default.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double Shape_Length = 0.0;

    /**
     * @brief Ordered vertices of the asset path or outline.
     *
     * ATempUIActor places the actor at the mean of these points, and
     * UEntityGeometryMeshComponent builds its mesh from them when mesh generation is
     * enabled.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FInfPtInfoRouteLnPoint> geometry;

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
 * @brief Ditto thing for one road route segment from the Infraestruturas de Portugal (IP)
 * `info-route-ln` data set.
 *
 * RegisterAllEntityTypes() maps thingIds containing `info-route-ln` to this struct,
 * with UInfoRouteLnExtension supplying the type-specific behavior. FJsonObjectConverter
 * fills it from the thing JSON, so field names match the JSON keys.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtInfoRouteLnData
{
    GENERATED_USTRUCT_BODY()

    /**
     * @brief Ditto thing ID; UDT4MOBEntityFactory picks this struct for IDs containing
     * `info-route-ln`.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    /** @brief ID of the Ditto policy that controls access to the thing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    /** @brief Asset data from the thing's `attributes` object. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtInfoRouteLnAttributes attributes;
};
