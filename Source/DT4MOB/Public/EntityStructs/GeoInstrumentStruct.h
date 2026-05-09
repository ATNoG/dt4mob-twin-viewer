#pragma once

#include "CoreMinimal.h"
#include "GeoInstrumentStruct.generated.h"

/**
 * @brief Enumeration-like struct for asset/instrument type classification.
 *
 * Shared by both geo-asset and instrument entities.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FGeoAssetTypeInfo
{
    GENERATED_USTRUCT_BODY()

    /** @brief Coded value string (asset database code, e.g. "ActivoGeotecnicoTipoActivo_Muro"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FString value = FString();

    /** @brief Human-readable display name (e.g. "Muro", "Inclinómetro"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FString name = FString();

    FString toString() const
    {
        return FString::Printf(TEXT("TypeInfo - Value: %s, Name: %s"), *value, *name);
    }
};

/**
 * @brief Geographic coordinates of an instrument sensor point.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FGeoInstrumentCoordinates
{
    GENERATED_USTRUCT_BODY()

    /** @brief Geographic latitude in decimal degrees (WGS-84). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    double latitude = 0.0;

    /** @brief Geographic longitude in decimal degrees (WGS-84). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    double longitude = 0.0;
};

/**
 * @brief Attributes of an instrument sensor attached to a geo asset.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FGeoInstrumentAttributes
{
    GENERATED_USTRUCT_BODY()

    /** @brief Instrument UUID. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FString instrumentoId = FString();

    /** @brief Instrument type classification (e.g. "Inclinómetro"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FGeoAssetTypeInfo tipoInstrumento;

    /** @brief Instrument registration code (e.g. "I1C"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FString matricula = FString();

    /** @brief Start chainage of the instrument position on the road (metres). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    int32 pkInicial = 0;

    /** @brief Geographic position of the instrument sensor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FGeoInstrumentCoordinates coordinates;

    FString toString() const
    {
        return FString::Printf(
            TEXT("GeoInstrumentAttributes - ID: %s, Matricula: %s, TipoInstrumento: %s, "
                 "PkInicial: %d, Lat: %.6f, Lon: %.6f"),
            *instrumentoId, *matricula, *tipoInstrumento.toString(),
            pkInicial, coordinates.latitude, coordinates.longitude);
    }
};

/**
 * @brief Properties of a single measurement parameter feature on an instrument.
 *
 * limiteAlerta, limiteAlarme, and latestValue default to 0.0 when JSON value is null.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FGeoInstrumentMeasurementProperties
{
    GENERATED_USTRUCT_BODY()

    /** @brief Human-readable parameter name (e.g. "Deslocamento Acumulado X 1"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FString parametroNome = FString();

    /** @brief Parameter key matching the feature map key (e.g. "DispX1"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FString parametroChave = FString();

    /** @brief Measurement unit (e.g. "mm"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FString unidades = FString();

    /** @brief Alert threshold value (0.0 when null/unset). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    double limiteAlerta = 0.0;

    /** @brief Alarm threshold value (0.0 when null/unset). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    double limiteAlarme = 0.0;

    /** @brief Most recent sensor reading (0.0 when null/unset). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    double latestValue = 0.0;

    FString toString() const
    {
        return FString::Printf(
            TEXT("Measurement - Chave: %s, Nome: %s, Unidades: %s, "
                 "Alert: %.4f, Alarm: %.4f, Latest: %.4f"),
            *parametroChave, *parametroNome, *unidades,
            limiteAlerta, limiteAlarme, latestValue);
    }
};

/**
 * @brief A single feature entry on an instrument, wrapping its measurement properties.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FGeoInstrumentFeatureEntry
{
    GENERATED_USTRUCT_BODY()

    /** @brief Measurement parameter data for this feature. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FGeoInstrumentMeasurementProperties properties;

    FString toString() const
    {
        return properties.toString();
    }
};

/**
 * @brief Root Ditto thing struct for a geotechnical instrument sensor.
 *
 * Maps directly from the Ditto JSON payload via FJsonObjectConverter.
 * thingId format: "geo-asset:<parent-uuid>.instrument.<instrument-uuid>"
 *
 * Features are keyed by parameter code (e.g. "DispX1", "DispX2") — the number
 * of entries varies per instrument type (inclinometer, piezometer, etc.).
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FGeoInstrumentData
{
    GENERATED_USTRUCT_BODY()

    /** @brief Ditto thing identifier (e.g. "geo-asset:<uuid>.instrument.<uuid>"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FString thingId = FString();

    /** @brief Ditto policy identifier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FString policyId = FString();

    /** @brief Static attributes of the instrument sensor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    FGeoInstrumentAttributes attributes;

    /**
     * @brief Live measurement features keyed by parameter code (e.g. "DispX1").
     *
     * Key count and names vary by instrument type.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GeoAsset")
    TMap<FString, FGeoInstrumentFeatureEntry> features;

    FString toString() const
    {
        return FString::Printf(TEXT("GeoInstrumentData - ThingId: %s, PolicyId: %s, Attributes: %s, Features: %d"),
                               *thingId, *policyId, *attributes.toString(), features.Num());
    }
};
