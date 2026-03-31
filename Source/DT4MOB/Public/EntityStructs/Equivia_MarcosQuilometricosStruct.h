#pragma once
#include "CoreMinimal.h"
#include "GeoStruct.h"
#include "Equivia_MarcosQuilometricosStruct.generated.h"

/**
 * @brief Static attributes of an Equivia kilometre marker (Marcos Quilométricos) entity.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FMarcosQuilometricosAttributes
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

    /** @brief Geographic location of the kilometre marker. */
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

    /** @brief Asset type string. */
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

    /** @brief Marker type classification code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 tipo = 0;

    /** @brief Kilometre reading displayed on the marker. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double quilometragem = 0;
};

/**
 * @brief Placeholder for future Ditto feature data on kilometre marker entities.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FMarcosQuilometricosFeatures
{
    GENERATED_USTRUCT_BODY()
};

/**
 * @brief Root Ditto thing struct for an Equivia kilometre marker entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FMarcosQuilometricosData
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
    FMarcosQuilometricosAttributes attributes;

    /** @brief Placeholder features block (currently empty). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FMarcosQuilometricosFeatures features;
};
