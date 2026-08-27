#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_InfoRouteLnStruct.generated.h"

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtInfoRouteLnPoint
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double latitude = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double longitude = 0.0;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtInfoRouteLnAttributes
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString route_name;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString roadnumber;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double kmi = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double kmf = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double FIRST_POINT = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double LAST_POINT = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double Shape_Length = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FInfPtInfoRouteLnPoint> geometry;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double geotile = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> closest_meteo_stations;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtInfoRouteLnData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtInfoRouteLnAttributes attributes;
};
