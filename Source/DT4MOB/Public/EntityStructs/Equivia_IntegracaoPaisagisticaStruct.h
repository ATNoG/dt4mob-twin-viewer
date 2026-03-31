#pragma once
#include "CoreMinimal.h"
#include "GeoStruct.h"
#include "Equivia_IntegracaoPaisagisticaStruct.generated.h"

/**
 * @brief Static attributes of an Equivia landscape integration (Integração Paisagística) entity.
 *
 * Represents roadside green areas, planted zones, or other landscaping assets.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FIntegracaoPaisagisticaAttributes
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

    /** @brief Array of geographic locations defining the landscaping area extent. */
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

    /** @brief Chainage (km) of the landscaping area along the road. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km = 0;

    /** @brief Asset type string. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString type;

    /** @brief Secondary road lane code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 via_auxiliar = 0;

    /** @brief Vegetation type/classification code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 vegetacao = 0;

    /** @brief True if an irrigation system is present. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool sistema_de_rega = false;

    /** @brief IDs of the nearest meteorology stations for weather/irrigation correlation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> closest_meteo_stations;
};

/**
 * @brief Placeholder for future Ditto feature data on landscape integration entities.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FIntegracaoPaisagisticaFeatures
{
    GENERATED_USTRUCT_BODY()
};

/**
 * @brief Root Ditto thing struct for an Equivia landscape integration entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FIntegracaoPaisagisticaData
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
    FIntegracaoPaisagisticaAttributes attributes;

    /** @brief Placeholder features block (currently empty). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FIntegracaoPaisagisticaFeatures features;
};
