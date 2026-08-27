#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_TaludesStruct.generated.h"

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtTaludesPoint
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double latitude = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double longitude = 0.0;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtTaludesAttributes
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ID = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double shape_Length = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double shape_Area = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FInfPtTaludesPoint> geometry;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double geotile = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> closest_meteo_stations;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtTaludesData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtTaludesAttributes attributes;
};
