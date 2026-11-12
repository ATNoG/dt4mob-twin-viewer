/**
 * @file
 * @brief Ditto JSON structs for Infraestruturas de Portugal road lighting things
 *        (`iluminacao`).
 * @ingroup entitystructs
 */
#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_IluminacaoStruct.generated.h"

/**
 * @brief WGS-84 position of a road lighting asset (`attributes.location`).
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtIluminacaoLocation
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
 * @brief Contents of the `attributes` object for a road lighting asset.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtIluminacaoAttributes
{
    GENERATED_USTRUCT_BODY()

    /**
     * @brief Status of the lighting asset, as text.
     *
     * Shown as "Estado" in the info tab by default.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString estado;

    /** @brief Number of light points on the asset, sent as a string. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString pontosdeluz;

    /** @brief Pole (column) height, sent as a string with unspecified units. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString alturacoluna;

    /** @brief Raw `tipodisprotecao` value (type of protection device). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString tipodisprotecao;

    /** @brief Location description, as text. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString localizacao;

    /** @brief Raw `posicao` (position) value, as text. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString posicao;

    /**
     * @brief Road the asset is on.
     *
     * Shown as "Via" in the info tab by default.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString via;

    /** @brief Portuguese district the asset is in. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString distrito;

    /** @brief Municipality (concelho) the asset is in. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString concelho;

    /** @brief Raw `gestao` (management) value, as text. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString gestao;

    /** @brief Raw `condicao_ativo` (asset condition) value, as text. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString condicao_ativo;

    /** @brief Record identifier of the asset in the IP source data set. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ID = 0;

    /** @brief WGS-84 position of the asset, used by ATempUIActor to place the actor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtIluminacaoLocation location;

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
     * @brief URL of the GLB model loaded for the actor.
     *
     * ATempUIActor::TryLoadGlbModel() uses `attributes.polygon` from the thing JSON
     * and falls back to this compiled-in street light model when the thing has none.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString polygon = TEXT("https://dt4mob.av.it.pt/s3/dt4mob-public/InfraestructureModels/Street_Light.glb");
};

/**
 * @brief Ditto thing for one road lighting asset from the Infraestruturas de Portugal (IP)
 * `iluminacao` data set.
 *
 * RegisterAllEntityTypes() maps thingIds containing `iluminacao` to this struct,
 * with UIluminacaoExtension supplying the type-specific behavior. FJsonObjectConverter
 * fills it from the thing JSON, so field names match the JSON keys.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtIluminacaoData
{
    GENERATED_USTRUCT_BODY()

    /**
     * @brief Ditto thing ID; UDT4MOBEntityFactory picks this struct for IDs containing
     * `iluminacao`.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    /** @brief ID of the Ditto policy that controls access to the thing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    /** @brief Asset data from the thing's `attributes` object. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtIluminacaoAttributes attributes;
};
