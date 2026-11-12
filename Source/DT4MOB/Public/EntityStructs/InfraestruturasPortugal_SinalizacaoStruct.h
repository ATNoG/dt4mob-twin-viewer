/**
 * @file
 * @brief Ditto JSON structs for Infraestruturas de Portugal road sign things
 *        (`sinalizacao`).
 * @ingroup entitystructs
 */
#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_SinalizacaoStruct.generated.h"

/**
 * @brief WGS-84 position of a road sign (`attributes.location`).
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtSinalizacaoLocation
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
 * @brief Contents of the `attributes` object for a road sign.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtSinalizacaoAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Record identifier of the asset in the IP source data set. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ID = 0;

    /**
     * @brief Raw `ID_SS` value (a second identifier from the source data set; meaning
     * not documented).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ID_SS = 0;

    /**
     * @brief Road sign code, for example `C13` or `D1A`.
     *
     * Shown as "Code" in the info tab by default. USinalizacaoModelComponent uses
     * it to pick the real sign face from the sign atlas; codes outside that catalog
     * fall back to the `polygon` GLB.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Code;

    /** @brief WGS-84 position of the asset, used by ATempUIActor to place the actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtSinalizacaoLocation location;

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

    /**
     * @brief URL of the fallback GLB sign model.
     *
     * ATempUIActor::TryLoadGlbModel() uses `attributes.polygon` from the thing JSON,
     * or this compiled-in default when the thing has none. Only loaded when
     * USinalizacaoModelComponent has no atlas art for `Code`.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString polygon = TEXT("https://dt4mob.av.it.pt/s3/dt4mob-public/InfraestructureModels/Sign.glb");
};

/**
 * @brief Ditto thing for one road sign from the Infraestruturas de Portugal (IP)
 * `sinalizacao` data set.
 *
 * RegisterAllEntityTypes() maps thingIds containing `sinalizacao` to this struct,
 * with USinalizacaoExtension supplying the type-specific behavior. FJsonObjectConverter
 * fills it from the thing JSON, so field names match the JSON keys.
 *
 * USinalizacaoModelComponent draws the sign from `attributes.Code` and falls back
 * to the `attributes.polygon` GLB for codes it has no art for.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtSinalizacaoData
{
    GENERATED_USTRUCT_BODY()

    /**
     * @brief Ditto thing ID; UDT4MOBEntityFactory picks this struct for IDs containing
     * `sinalizacao`.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    /** @brief ID of the Ditto policy that controls access to the thing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    /** @brief Asset data from the thing's `attributes` object. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtSinalizacaoAttributes attributes;
};
