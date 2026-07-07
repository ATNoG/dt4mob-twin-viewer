#pragma once
#include "CoreMinimal.h"
#include "InfraestruturasPortugal_IluminacaoStruct.generated.h"

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtIluminacaoLocation
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double latitude = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double longitude = 0.0;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtIluminacaoAttributes
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString estado;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString pontosdeluz;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString alturacoluna;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString tipodisprotecao;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString localizacao;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString posicao;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString via;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString distrito;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString concelho;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString gestao;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString condicao_ativo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ID = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtIluminacaoLocation location;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double geotile = 0.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> closest_meteo_stations;
};

USTRUCT(BlueprintType)
struct DT4MOB_API FInfPtIluminacaoData
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString thingId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString policyId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FInfPtIluminacaoAttributes attributes;
};
