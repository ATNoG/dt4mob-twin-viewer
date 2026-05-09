#pragma once

#include "CoreMinimal.h"
#include "GeoInstrumentStruct.h"
#include "GeoAssetStruct.generated.h"

/**
 * @brief Attributes of a geotechnical infrastructure asset (e.g. retaining wall, slope).
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FGeoAssetAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Asset database UUID. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FString id = FString();

    /** @brief Asset registration number (e.g. "A9 MA Cd 026+290"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FString matricula = FString();

    /** @brief Human-readable location description (e.g. "Loures"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FString localizacao = FString();

    /** @brief Geotechnical asset type classification. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FGeoAssetTypeInfo tipoActivo;

    /** @brief Geographic latitude of the asset centroid in decimal degrees (WGS-84). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    double latitude = 0.0;

    /** @brief Geographic longitude of the asset centroid in decimal degrees (WGS-84). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    double longitude = 0.0;

    /** @brief Current condition index (0.0 = worst, 5.0 = best). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    double indiceCondicaoAtual = 0.0;

    /** @brief Reliability index of the condition assessment (0–100). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    int32 indiceFiabilidade = 0;

    /** @brief Asset height in metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    double altura = 0.0;

    /** @brief Asset length (extension) in metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    int32 extensao = 0;

    /** @brief Asset inclination in degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    int32 inclinacaoGraus = 0;

    /** @brief Start chainage on the operational road (metres). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    int32 pkExploracaoInicial = 0;

    /** @brief End chainage on the operational road (metres). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    int32 pkExploracaoFinal = 0;

    /** @brief Start chainage on the project baseline (metres). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    int32 pkProjectInicial = 0;

    /** @brief End chainage on the project baseline (metres). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    int32 pkProjectFinal = 0;

    /**
     * @brief IDs of instruments attached to this asset.
     *
     * Each entry is a bare UUID suffix used to build the full instrument thingId:
     * "geo-asset:<parent-uuid>.instrument.<entry>".
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    TArray<FString> instrumentList;

    FString toString() const
    {
        return FString::Printf(
            TEXT("GeoAssetAttributes - ID: %s, Matricula: %s, Localizacao: %s, TipoActivo: %s, "
                 "Lat: %.6f, Lon: %.6f, ConditionIndex: %.4f, Reliability: %d, "
                 "Altura: %.2f, Extensao: %d, Inclinacao: %d, "
                 "PKExploracao: %d-%d, PKProject: %d-%d, Instruments: %d"),
            *id, *matricula, *localizacao, *tipoActivo.toString(),
            latitude, longitude, indiceCondicaoAtual, indiceFiabilidade,
            altura, extensao, inclinacaoGraus,
            pkExploracaoInicial, pkExploracaoFinal,
            pkProjectInicial, pkProjectFinal,
            instrumentList.Num());
    }
};

/**
 * @brief Root Ditto thing struct for a geotechnical infrastructure asset.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 * thingId format: "geo-asset:<uuid>"
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FGeoAssetData
{
    GENERATED_USTRUCT_BODY()

    /** @brief Ditto thing identifier (e.g. "geo-asset:e27a9388-..."). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FString thingId = FString();

    /** @brief Ditto policy identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FString policyId = FString();

    /** @brief Static attributes from the asset database. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FGeoAssetAttributes attributes;

    FString toString() const
    {
        return FString::Printf(TEXT("GeoAssetData - ThingId: %s, PolicyId: %s, Attributes: %s"),
                               *thingId, *policyId, *attributes.toString());
    }
};
