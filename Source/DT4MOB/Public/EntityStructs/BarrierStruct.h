#pragma once

#include "CoreMinimal.h"
#include "BarrierStruct.generated.h"

/**
 * @brief A single WGS-84 coordinate belonging to a barrier geometry polyline.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FBarrierGeometry
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
 * @brief Placeholder for future Ditto feature data on barrier entities.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FBarrierFeatures
{
    GENERATED_USTRUCT_BODY()

    /** @brief Returns a placeholder string (no features currently defined). */
    FString toString() const
    {
        return FString::Printf(TEXT("No features available"));
    }
};

/**
 * @brief Static attributes of a road barrier (guardrail) entity from the Equivia asset database.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FBarrierAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief GIS object identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    int32 objectID = 0;

    /** @brief Barrier condition/state code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    int32 estado_barreira = 0;

    /** @brief Barrier type classification string. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    FString tipo_barreira = FString();

    /** @brief Manufacturer code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    int32 fabricante = 0;

    /** @brief Containment level code (N1–N4). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    int32 n_contencao = 0;

    /** @brief Working width code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    int32 n_larg_util = 0;

    /** @brief Location code (roadside position). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    int32 localizacao = 0;

    /** @brief Road lane position code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    int32 posicao_via = 0;

    /** @brief Guard height code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    int32 alt_guarda = 0;

    /** @brief Post spacing in metres. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    int32 dist_prumos = 0;

    /** @brief True if an associated drainage system exists. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    bool drenagem_assoc = false;

    /** @brief DPM (asset management) code. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    int32 dpm = 0;

    /** @brief Number of individual barrier elements in this record. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    int32 numero_barreiras = 0;

    /** @brief Start chainage (km). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    float kmi = 0.0f;

    /** @brief End chainage (km). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    float kmf = 0.0f;

    /** @brief District name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    FString distrito = FString();

    /** @brief Municipality name. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    FString concelho = FString();

    /** @brief True if the barrier is associated with a bridge or art structure. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    bool obra_de_arte = false;

    /** @brief True if the barrier record is currently active. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    bool condicao_ativo = false;

    /** @brief Array of WGS-84 coordinate points defining the barrier polyline. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    TArray<FBarrierGeometry> geometry;

    /** @brief IDs of the nearest meteorology stations for weather correlation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    TArray<FString> closest_meteo_stations;

    /** @brief Integer geotile identifier for spatial indexing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    int32 geotile_int = 0;

    /** @brief String representation of the geotile identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    FString geotile_str = FString();

    /** @brief Returns a human-readable summary of all barrier attributes. */
    FString toString() const
    {
        return FString::Printf(TEXT("Attributes - objectID: %d, estado_barreira: %d, tipo_barreira: %s, fabricante: %d, n_contencao: %d, n_larg_util: %d, localizacao: %d, posicao_via: %d, alt_guarda: %d, dist_prumos: %d, drenagem_assoc: %s, dpm: %d, numero_barreiras: %d, kmi: %.2f, kmf: %.2f, distrito: %s, concelho: %s, obra_de_arte: %s, condicao_ativo: %s, geometry: %s, closest_meteo_stations: %s"),
                               objectID, estado_barreira, *tipo_barreira, fabricante, n_contencao, n_larg_util, localizacao, posicao_via, alt_guarda, dist_prumos,
                               drenagem_assoc ? TEXT("true") : TEXT("false"), dpm, numero_barreiras, kmi, kmf,
                               *distrito, *concelho,
                               obra_de_arte ? TEXT("true") : TEXT("false"),
                               condicao_ativo ? TEXT("true") : TEXT("false"),
                               *FString::JoinBy(geometry, TEXT("; "), [](const FBarrierGeometry &Geo)
                                                { return Geo.toString(); }),
                               *FString::Join(closest_meteo_stations, TEXT(", ")));
    }
};

/**
 * @brief Root Ditto thing struct for a road barrier entity.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FBarrierData
{
    GENERATED_USTRUCT_BODY()

    /** @brief Ditto thing identifier (e.g. "barrier:12345"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    FString thingId;

    /** @brief Ditto policy identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    FString policyId;

    /** @brief Static attributes from the asset database. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    FBarrierAttributes attributes;

    /** @brief Placeholder features block (currently empty). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Barrier")
    FBarrierFeatures features;

    /** @brief Returns a human-readable summary of the full barrier thing. */
    FString toString() const
    {
        return FString::Printf(TEXT("Barrier Data - thingId: %s, policyId: %s, attributes: %s, features: %s"),
                               *thingId, *policyId, *attributes.toString(), *features.toString());
    }
};
