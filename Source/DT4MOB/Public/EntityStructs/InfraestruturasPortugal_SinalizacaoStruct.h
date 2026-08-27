#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_SinalizacaoStruct.generated.h"

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtSinalizacaoLocation
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double latitude = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double longitude = 0.0;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtSinalizacaoAttributes
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ID = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ID_SS = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Code;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtSinalizacaoLocation location;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double geotile = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> closest_meteo_stations;

    /** @brief GLB model URL. Falls back to this default when Ditto does not provide one. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString polygon = TEXT("https://dt4mob.av.it.pt/s3/dt4mob-public/InfraestructureModels/Sign.glb");
};

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtSinalizacaoData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtSinalizacaoAttributes attributes;
};
