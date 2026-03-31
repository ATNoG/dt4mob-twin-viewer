#pragma once
#include "CoreMinimal.h"
#include "GeoStruct.h"
#include "Equivia_VedacoesStruct.generated.h"

/**
 * @brief Static attributes of an Equivia fencing/enclosure (Vedações) entity.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FVedacoesAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief GIS object identifier (stored as string in this entity type). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString object_id;

    /** @brief Road category code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 categoria_via = 0;

    /** @brief Position code relative to the road. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 posicao = 0;

    /** @brief Array of geographic locations defining the fencing polyline. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FGeoLocation> location;

    /** @brief Municipality (concelho) name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString concelho;

    /** @brief District (distrito) name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString distrito;

    /** @brief Road identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString estrada;

    /** @brief Start chainage (km) of the fencing section. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km = 0;

    /** @brief End chainage (km) of the fencing section. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km_fim = 0;

    /** @brief True if the asset record is currently active. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool condicao_ativo = false;

    /** @brief Fencing type/material classification string. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString type;

    /** @brief Secondary road lane code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 via_auxiliar = 0;

    /** @brief Height of the fencing in metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 altura = 0;

    /** @brief Number of gates (portões) in the fencing section. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 n_portoes = 0;

    /** @brief Fencing sub-type/style code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 vedacoes = 0;

    /** @brief IDs of the nearest meteorology stations for weather correlation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> closest_meteo_stations;
};

/**
 * @brief Placeholder for future Ditto feature data on fencing entities.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FVedacoesFeatures
{
    GENERATED_USTRUCT_BODY()
};

/**
 * @brief Root Ditto thing struct for an Equivia fencing/enclosure entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FVedacoesData
{
    GENERATED_USTRUCT_BODY()

    /** @brief Ditto thing identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    /** @brief Ditto policy identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    /** @brief Static asset attributes from the Equivia database. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVedacoesAttributes attributes;

    /** @brief Placeholder features block (currently empty). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVedacoesFeatures features;
};
