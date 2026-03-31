#pragma once
#include "CoreMinimal.h"
#include "GeoStruct.h"
#include "Equivia_SeccoesStruct.generated.h"

/**
 * @brief Static attributes of an Equivia road section (Secções) entity.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FSeccoesAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief GIS object identifier (stored as string in this entity type). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString object_id;

    /** @brief Array of geographic locations defining the section polyline. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FGeoLocation> location;

    /** @brief District (distrito) name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString distrito;

    /** @brief Road identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString estrada;

    /** @brief Start chainage (km) of the section. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km_ini = 0;

    /** @brief End chainage (km) of the section. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double km_fim = 0;

    /** @brief Section type/classification string. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString type;

    /** @brief Human-readable name of the road section. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString nome;

    /** @brief Node number at the start of the section. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 no_ini = 0;

    /** @brief Node number at the end of the section. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 no_fim = 0;

    /** @brief IDs of the nearest meteorology stations for weather correlation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> closest_meteo_stations;
};

/**
 * @brief Placeholder for future Ditto feature data on road section entities.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FSeccoesFeatures
{
    GENERATED_USTRUCT_BODY()
};

/**
 * @brief Root Ditto thing struct for an Equivia road section entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FSeccoesData
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
    FSeccoesAttributes attributes;

    /** @brief Placeholder features block (currently empty). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FSeccoesFeatures features;
};
