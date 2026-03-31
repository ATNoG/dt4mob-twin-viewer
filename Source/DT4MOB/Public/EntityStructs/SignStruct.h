#pragma once

#include "CoreMinimal.h"
#include "SignStruct.generated.h"

/**
 * @brief Geographic location sub-struct used by road sign entities.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FStructLocation
{
    GENERATED_USTRUCT_BODY()

    /** @brief Geographic longitude in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
    double longitude = 0.0;

    /** @brief Geographic latitude in decimal degrees. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
    double latitude = 0.0;

    /** @brief Returns a human-readable coordinate string. */
    FString toString() const
    {
        return FString::Printf(TEXT("(Longitude: %.6f, Latitude: %.6f)"), longitude, latitude);
    }
};

/**
 * @brief Static attributes of a road sign entity from the Equivia asset database.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FSignAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Sign category/type string. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString type = FString();

    /** @brief GIS object identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    double objectID = 0.0;

    /** @brief RST (Road Safety Tool) code for the sign. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    double codRST = 0.0;

    /** @brief Sign shape classification (e.g. triangular, circular). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString formaSinal = FString();

    /** @brief Physical condition of the sign. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString estado = FString();

    /** @brief IP sign code identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    int32 codipsinal = 0;

    /** @brief Road identifier where the sign is located. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString via = FString();

    /** @brief Direction of travel the sign applies to. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString sentidoVia = FString();

    /** @brief Road environment type (urban/rural/motorway). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString tipoAmbRod = FString();

    /** @brief Physical position relative to the roadway. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString posicao = FString();

    /** @brief Retroreflective face class. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString classeRetroFace = FString();

    /** @brief Sign substrate/backing material. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString substrato = FString();

    /** @brief Sign height in millimetres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    int altura = 0;

    /** @brief Sign width in millimetres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    int largura = 0;

    /** @brief Manufacturer name or code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString fabricante = FString();

    /** @brief District name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString distrito = FString();

    /** @brief Sign face identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString faced = FString();

    /** @brief Reason for substitution (if applicable). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString motSubst = FString();

    /** @brief Management entity responsible for the sign. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString gestao = FString();

    /** @brief European classification code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString ec = FString();

    /** @brief Chainage (km) of the sign along the road. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    double pk = 0.0;

    /** @brief Geographic position of the sign. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FStructLocation location;

    /** @brief IDs of the nearest meteorology stations for weather correlation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    TArray<FString> closest_meteo_stations;

    /** @brief Integer geotile identifier for spatial indexing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    int32 geotile_int = 0;

    /** @brief String representation of the geotile identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString geotile_str = FString();

    /** @brief Returns a human-readable summary of all sign attributes. */
    FString toString() const
    {
        return FString::Printf(TEXT("Type: %s, ObjectID: %.2f, CodRST: %.2f, FormaSinal: %s, Estado: %s, Codipsinal: %d, Via: %s, SentidoVia: %s, TipoAmbRod: %s, Posicao: %s, ClasseRetroFace: %s, Substrato: %s, Altura: %d, Largura: %d, Fabricante: %s, Distrito: %s, Faced: %s, MotSubst: %s, Gestao: %s, EC: %s, PK: %.2f, Location: %s, ClosestMeteoStationsCount: %d"),
                               *type,
                               objectID,
                               codRST,
                               *formaSinal,
                               *estado,
                               codipsinal,
                               *via,
                               *sentidoVia,
                               *tipoAmbRod,
                               *posicao,
                               *classeRetroFace,
                               *substrato,
                               altura,
                               largura,
                               *fabricante,
                               *distrito,
                               *faced,
                               *motSubst,
                               *gestao,
                               *ec,
                               pk,
                               *location.toString(),
                               closest_meteo_stations.Num());
    };
};

/**
 * @brief Placeholder for future Ditto feature data on road sign entities.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FSignFeatures
{
    GENERATED_USTRUCT_BODY()

    /** @brief Returns a placeholder string (no features currently defined). */
    FString toString() const
    {
        return FString::Printf(TEXT("SignFeatures - No specific features defined yet."));
    }
};

/**
 * @brief Root Ditto thing struct for a road sign entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FSignData
{
    GENERATED_USTRUCT_BODY()

    /** @brief Ditto thing identifier (e.g. "sign:12345"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString thingId = FString();

    /** @brief Ditto policy identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FString policyId = FString();

    /** @brief Static sign attributes from the asset database. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FSignAttributes attributes;

    /** @brief Placeholder features block (currently empty). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sign")
    FSignFeatures features;

    /** @brief Returns a human-readable summary of the full sign thing. */
    FString toString() const
    {
        return FString::Printf(TEXT("SignData - ThingId: %s, PolicyId: %s, Attributes: %s, Features: %s"),
                               *thingId, *policyId, *attributes.toString(), *features.toString());
    }
};
