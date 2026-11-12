/**
 * @file
 * @brief Schema for the old Equivia road section (seccoes) data set.
 * @deprecated Not included or registered anywhere. The Infraestruturas de Portugal
 *             structs in EntityStructs/ replaced the Equivia data sets; kept for reference.
 * @ingroup entitystructs_deprecated
 */
#pragma once
#include "CoreMinimal.h"
#include "../GeoStruct.h"
#include "Equivia_SeccoesStruct.generated.h"

/**
 * @brief Static attributes of an Equivia road section (Secções) entity.
 * @ingroup entitystructs_deprecated
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
 * @ingroup entitystructs_deprecated
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
 * @ingroup entitystructs_deprecated
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

    /** @brief Display name shown in the entity type dropdown. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString DisplayName = TEXT("Road Section");

    /** @brief If true, show a warning that this entity type has no server-side handling. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bNoServerHandling = true;

    /**
     * @brief Builds a default instance placed at the given position.
     * @param Lat Latitude in decimal degrees.
     * @param Lon Longitude in decimal degrees.
     * @return The new instance; every other field keeps its default value.
     */
    static FSeccoesData MakeDefault(double Lat, double Lon)
    {
        FSeccoesData Data;
        FGeoLocation Point;
        Point.latitude = Lat;
        Point.longitude = Lon;
        Data.attributes.location.Add(Point);
        return Data;
    }
};
