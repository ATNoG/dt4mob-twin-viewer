// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SignSpawnStrategy.h"
#include "SignSpawnStrategy_PerSign.generated.h"

class UStaticMesh;

/**
 * @brief Naive baseline: one fully self-contained, pre-baked mesh+texture per
 * sign code, no ISM/instancing engineering at all. Expected to lose on disk
 * size (duplicated geometry per code) and draw calls versus the other two
 * strategies — that's the point of including it.
 */
UCLASS()
class DT4MOB_API USignSpawnStrategy_PerSign : public USignSpawnStrategy
{
    GENERATED_BODY()

public:
    virtual void Execute(const TArray<FString> &CodeSequence, TFunction<void(bool)> OnComplete) override;
    virtual void Clear() override;

private:
    UPROPERTY()
    TMap<FString, TObjectPtr<UStaticMesh>> MeshByCode;
};
