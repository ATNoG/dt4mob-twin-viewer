// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SignSpawnStrategy.h"
#include "SignSpawnStrategy_PerTexture.generated.h"

class UStaticMesh;
class UTexture2D;
class UMaterialInterface;
class UInstancedStaticMeshComponent;

/**
 * @brief Shared mesh per shape (same as AtlasMaterial), but one Material
 * Instance Dynamic + one ISM per distinct sign texture, since UE can't batch
 * different materials into one ISM. Headline contrast vs AtlasMaterial: same
 * geometry/instance count, but N components/materials/draws instead of 2.
 */
UCLASS()
class DT4MOB_API USignSpawnStrategy_PerTexture : public USignSpawnStrategy
{
    GENERATED_BODY()

public:
    /** @brief Parent material with a TextureSampleParameter2D named "SignTex". Must have "Used with Instanced Static Meshes" checked. */
    UPROPERTY()
    TObjectPtr<UMaterialInterface> TexturedBaseMaterial;

    virtual void Execute(const TArray<FString> &CodeSequence, TFunction<void(bool)> OnComplete) override;
    virtual void Clear() override;

private:
    UPROPERTY()
    TMap<FString, TObjectPtr<UStaticMesh>> MeshByShape;

    UPROPERTY()
    TMap<FString, TObjectPtr<UTexture2D>> TexByCode;

    UPROPERTY()
    TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> ISMByCode;
};
