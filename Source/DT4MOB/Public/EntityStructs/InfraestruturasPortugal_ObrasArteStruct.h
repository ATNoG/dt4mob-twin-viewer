#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_ObrasArteStruct.generated.h"

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtObrasArtePoint
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double latitude = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double longitude = 0.0;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtObrasArteAttributes
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ID = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double shape_Length = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double shape_Area = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FInfPtObrasArtePoint> geometry;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double geotile = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> closest_meteo_stations;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtObrasArteData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtObrasArteAttributes attributes;
};
