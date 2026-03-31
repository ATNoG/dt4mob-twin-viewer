#pragma once

#include "CoreMinimal.h"
#include "TaludeStruct.generated.h"

/**
 * @brief Enumeration-like struct representing the active-type classification of a slope (talude).
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTaludeActiveType
{
    GENERATED_USTRUCT_BODY()

    /** @brief Coded value string (asset database code). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    FString value = FString();

    /** @brief Human-readable display name of the active type. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    FString name = FString();

    /** @brief Returns a human-readable summary of the active type. */
    FString toString() const
    {
        return FString::Printf(TEXT("TaludeActiveType - Value: %s, Name: %s"), *value, *name);
    }
};

/**
 * @brief Static attributes of a slope (talude / embankment) entity from the Equivia asset database.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTaludeAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Asset database identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    FString id = FString();

    /** @brief Asset registration number. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    FString matricula = FString();

    /** @brief Human-readable location description. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    FString localizacao = FString();

    /** @brief Active/asset type classification. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    FTaludeActiveType tipoActivo;

    /** @brief Reliability index of the condition assessment (0–5). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    int32 indiceFiabilidade = 0;

    /** @brief Start chainage (pk) of the slope along the operational road (metres). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    int32 pkExploracaoInicial = 0;

    /** @brief End chainage (pk) of the slope along the operational road (metres). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    int32 pkExploracaoFinal = 0;

    /** @brief Geographic latitude of the slope centroid in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    double latitude = 0.0;

    /** @brief Geographic longitude of the slope centroid in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    double longitude = 0.0;

    /** @brief Slope height in metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    double altura = 0.0;

    /** @brief Slope length (extension) in metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    int32 extensao = 0;

    /** @brief Slope inclination in degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    int32 inclinacaoGraus = 0;

    /** @brief Start chainage (pk) on the project baseline (metres). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    int32 pkProjectInicial = 0;

    /** @brief End chainage (pk) on the project baseline (metres). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    int32 pkProjectFinal = 0;

    /** @brief Current condition index (0.0 = worst, 5.0 = best). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    double indiceCondicaoAtual = 0.0;

    /** @brief Returns a human-readable summary of all talude attributes. */
    FString toString() const
    {
        return FString::Printf(TEXT("TaludeAttributes - ID: %s, Matricula: %s, Localizacao: %s, TipoActivo: %s, IndiceFiabilidade: %d, PKExploracaoInicial: %d, PKExploracaoFinal: %d, Latitude: %.6f, Longitude: %.6f, Altura: %.2f, Extensao: %d, InclinacaoGraus: %d, PKProjectInicial: %d, PKProjectFinal: %d, IndiceCondicaoAtual: %.2f"),
                               *id,
                               *matricula,
                               *localizacao,
                               *tipoActivo.toString(),
                               indiceFiabilidade,
                               pkExploracaoInicial,
                               pkExploracaoFinal,
                               latitude,
                               longitude,
                               altura,
                               extensao,
                               inclinacaoGraus,
                               pkProjectInicial,
                               pkProjectFinal,
                               indiceCondicaoAtual);
    }
};

/**
 * @brief Placeholder for future Ditto feature data on talude entities.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTaludeFeatures
{
    GENERATED_USTRUCT_BODY()

    /** @brief Returns a placeholder string (no features currently defined). */
    FString toString() const
    {
        return FString::Printf(TEXT("TaludeFeatures - No specific features defined"));
    }
};

/**
 * @brief Root Ditto thing struct for a slope/embankment (talude/muro-talude) entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FTaludeData
{
    GENERATED_USTRUCT_BODY()

    /** @brief Ditto thing identifier (e.g. "muro-talude:12345"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    FString thingId = FString();

    /** @brief Ditto policy identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    FString policyId = FString();

    /** @brief Ditto thing definition URI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    FString definition = FString();

    /** @brief Placeholder features block (currently empty). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    FTaludeFeatures features;

    /** @brief Static attributes from the asset database. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Talude")
    FTaludeAttributes attributes;

    /** @brief Returns a human-readable summary of the full talude thing. */
    FString toString() const
    {
        return FString::Printf(TEXT("TaludeData - ThingId: %s, PolicyId: %s, Definition: %s, Features: %s, Attributes: %s"),
                               *thingId,
                               *policyId,
                               *definition,
                               *features.toString(),
                               *attributes.toString());
    }
};
