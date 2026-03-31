#pragma once

#include "CoreMinimal.h"
#include "GeoStruct.h"
#include "Equivia_TaludesStruct.generated.h"

/**
 * @brief Static attributes of an Equivia slope/embankment (Taludes) entity from the Equivia asset database.
 *
 * @note This is distinct from FTaludeAttributes (TaludeStruct.h), which comes from
 *       the standalone "muro-talude" Ditto namespace. This struct is used by the
 *       "equivia:Taludes" thing type and contains additional road-alignment fields.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FEquiviaTaludesAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief GIS object identifier (stored as string in this entity type). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString object_id;

    /** @brief Road category code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 categoria_via = 0;

    /** @brief Position code relative to the road (cut/fill side). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 posicao = 0;

    /** @brief Array of geographic locations defining the slope polyline. */
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

    /** @brief Reference chainage (km) of the slope. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km = 0.0;

    /** @brief Start chainage (km) of the slope. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km_ini = 0.0;

    /** @brief End chainage (km) of the slope. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km_fim = 0.0;

    /** @brief Management entity code responsible for the slope. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 gestao = 0;

    /** @brief True if the asset record is currently active. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool condicao_ativo = false;

    /** @brief Asset type classification string. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString type;

    /** @brief Integer geotile identifier for spatial indexing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double geotile_int = 0.0;

    /** @brief String representation of the geotile identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString geotile_str;

    /** @brief Secondary road lane code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 via_auxiliar = 0;

    /** @brief Number of lanes adjacent to the slope. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 n_via = 0;

    /** @brief Slope inclination code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 inclinacao = 0;

    /** @brief Number of horizontal benches (banquetas) on the slope face. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 n_banquetas = 0;

    /** @brief Width of each bench (banqueta) in metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 largura_banquetas = 0;

    /** @brief True if erosion rills or gullies (regueiras/ravinamentos) are present. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool regueiras_ravinamentos = false;
};

/**
 * @brief Placeholder for future Ditto feature data on Equivia taludes entities.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FEquiviaTaludesFeatures
{
    GENERATED_USTRUCT_BODY()
};

/**
 * @brief Root Ditto thing struct for an Equivia slope/embankment entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FEquiviaTaludesData
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
    FEquiviaTaludesAttributes attributes;

    /** @brief Placeholder features block (currently empty). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FEquiviaTaludesFeatures features;
};
