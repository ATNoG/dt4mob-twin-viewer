#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_MarcasPoligonosStruct.generated.h"

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtMarcasPoligonosPoint
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double latitude = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double longitude = 0.0;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtMarcasPoligonosAttributes
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ID = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Code;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double SHAPE_Length = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double SHAPE_Area = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FInfPtMarcasPoligonosPoint> geometry;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double geotile = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> closest_meteo_stations;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtMarcasPoligonosData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtMarcasPoligonosAttributes attributes;
};
