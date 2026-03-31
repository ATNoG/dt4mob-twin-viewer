#pragma once
#include "CoreMinimal.h"
#include "GeoStruct.h"
#include "Equivia_IluminacaoStruct.generated.h"

/**
 * @brief Static attributes of an Equivia road lighting (Iluminação) entity.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FIluminacaoAttributes
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

    /** @brief Array of geographic locations defining the lighting installation extent. */
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

    /** @brief Start chainage (km) of the lighting installation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km = 0;

    /** @brief End chainage (km) of the lighting installation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km_fim = 0;

    /** @brief True if the asset record is currently active. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool condicao_ativo = false;

    /** @brief Asset type string. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString type;

    /** @brief Integer geotile identifier for spatial indexing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double geotile_int = 0;

    /** @brief String representation of the geotile identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString geotile_str;

    /** @brief Lighting fixture/pole material type code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 tipo_material = 0;

    /** @brief Total number of lighting devices in the installation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 n_dispositivos = 0;

    /** @brief Device/lamp type code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 dispositivos = 0;
};

/**
 * @brief Placeholder for future Ditto feature data on road lighting entities.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FIluminacaoFeatures
{
    GENERATED_USTRUCT_BODY()
};

/**
 * @brief Root Ditto thing struct for an Equivia road lighting entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FIluminacaoData
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
    FIluminacaoAttributes attributes;

    /** @brief Placeholder features block (currently empty). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIluminacaoFeatures features;
};
