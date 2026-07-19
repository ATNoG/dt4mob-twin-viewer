#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_CoordenadasProjetoAprovadasStruct.generated.h"

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtCoordenadasProjetoAprovadasLocation
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double latitude = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double longitude = 0.0;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtCoordenadasProjetoAprovadasAttributes
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Field1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double X = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double Y = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double Z = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtCoordenadasProjetoAprovadasLocation location;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double geotile = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> closest_meteo_stations;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtCoordenadasProjetoAprovadasData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtCoordenadasProjetoAprovadasAttributes attributes;
};
