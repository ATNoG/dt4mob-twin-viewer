#pragma once
#include "CoreMinimal.h"
#include "GeoStruct.h"
#include "Equivia_PavimentosStruct.generated.h"

/**
 * @brief Static attributes of an Equivia road pavement (Pavimentos) entity.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FPavimentosAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief GIS object identifier (stored as string in this entity type). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString object_id;

    /** @brief Array of geographic locations defining the pavement section polyline. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FGeoLocation> location;

    /** @brief District (distrito) name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString distrito;

    /** @brief Road identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString estrada;

    /** @brief Start chainage (km) of the pavement section. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km_ini = 0;

    /** @brief End chainage (km) of the pavement section. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km_fim = 0;

    /** @brief True if the asset record is currently active. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool condicao_ativo = false;

    /** @brief Pavement type/surface classification string. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString type;

    /** @brief IDs of the nearest meteorology stations for weather correlation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> closest_meteo_stations;
};

/**
 * @brief Placeholder for future Ditto feature data on pavement entities.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FPavimentosFeatures
{
    GENERATED_USTRUCT_BODY()
};

/**
 * @brief Root Ditto thing struct for an Equivia road pavement entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FPavimentosData
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
    FPavimentosAttributes attributes;

    /** @brief Placeholder features block (currently empty). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FPavimentosFeatures features;
};
