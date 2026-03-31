#pragma once
#include "CoreMinimal.h"
#include "GeoStruct.h"
#include "Equivia_DrenagemPontualStruct.generated.h"

/**
 * @brief Static attributes of an Equivia point drainage (Drenagem Pontual) entity.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FDrenagemPontualAttributes
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

    /** @brief Geographic location of the drainage element. */
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

    /** @brief Chainage (km) of the drainage element along the road. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km = 0.0;

    /** @brief True if the asset record is currently active. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool condicao_ativo = false;

    /** @brief Drainage element type string. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString type;

    /** @brief Integer geotile identifier for spatial indexing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double geotile_int = 0;

    /** @brief String representation of the geotile identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString geotile_str;

    /** @brief Secondary road lane code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 via_auxiliar = 0;

    /** @brief Material type code of the drainage element. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 tipo_material = 0;

    /** @brief Length dimension of the drainage element in metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double dimensoes_comprimento = 0;

    /** @brief Width dimension of the drainage element in metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double dimensoes_largura = 0;

    /** @brief Diameter of the drainage element in metres (for circular cross-sections). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double dimensoes_diametro = 0;
};

/**
 * @brief Placeholder for future Ditto feature data on point drainage entities.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FDrenagemPontualFeatures
{
    GENERATED_USTRUCT_BODY()
};

/**
 * @brief Root Ditto thing struct for an Equivia point drainage entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FDrenagemPontualData
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
    FDrenagemPontualAttributes attributes;

    /** @brief Placeholder features block (currently empty). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FDrenagemPontualFeatures features;
};
