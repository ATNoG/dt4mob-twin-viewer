#pragma once
#include "CoreMinimal.h"
#include "GeoStruct.h"
#include "Equivia_AcessosServentiasStruct.generated.h"

/**
 * @brief Static attributes of an Equivia access/service road (Acessos e Serventias) entity.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FAcessosServentiasAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief GIS object identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 object_id = 0;

    /** @brief Road category code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 categoria_via = 0;

    /** @brief Position code relative to the road. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 posicao = 0;

    /** @brief Geographic location of the access/service road. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGeoLocation location;

    /** @brief Municipality (concelho) name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString concelho;

    /** @brief District (distrito) name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString distrito;

    /** @brief Road identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString estrada;

    /** @brief Chainage (km) of the asset along the road. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km = 0.0;

    /** @brief Start chainage (km) of the asset extent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km_ini = 0.0;

    /** @brief End chainage (km) of the asset extent. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km_fim = 0.0;

    /** @brief True if the asset record is currently active. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool condicao_ativo = false;

    /** @brief Asset type string. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString type;

    /** @brief Width of the access road in metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 largura = 0;

    /** @brief Material type code of the access road surface. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 tipo_material = 0;

    /** @brief IDs of the nearest meteorology stations for weather correlation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> closest_meteo_stations;
};

/**
 * @brief Placeholder for future Ditto feature data on access/service road entities.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FAcessosServentiasFeatures
{
    GENERATED_USTRUCT_BODY()
};

/**
 * @brief Root Ditto thing struct for an Equivia access/service road entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FAcessosServentiasData
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
    FAcessosServentiasAttributes attributes;

    /** @brief Placeholder features block (currently empty). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FAcessosServentiasFeatures features;
};
