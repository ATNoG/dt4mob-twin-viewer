// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SignSpawnStrategy.h"
#include "SignSpawnStrategy_Atlas.generated.h"

class UStaticMesh;
class UTexture2D;
class UMaterialInterface;
class UInstancedStaticMeshComponent;

/**
 * @brief One shared mesh + one shared atlas-textured material per shape, all
 * instances of that shape batched into a single ISM, with each instance's
 * per-instance custom data (4 floats: u_offset, v_offset, u_scale, v_scale)
 * selecting which atlas cell it shows.
 */
UCLASS()
class DT4MOB_API USignSpawnStrategy_Atlas : public USignSpawnStrategy
{
    GENERATED_BODY()

public:
    /** @brief Parent material with a TextureSampleParameter2D named "AtlasTex" and PerInstanceCustomData-driven UV remap. Must have "Used with Instanced Static Meshes" checked. */
    UPROPERTY()
    TObjectPtr<UMaterialInterface> AtlasBaseMaterial;

    virtual void Execute(const TArray<FString> &CodeSequence, TFunction<void(bool)> OnComplete) override;
    virtual void Clear() override;

private:
    UPROPERTY()
    TMap<FString, TObjectPtr<UStaticMesh>> MeshByShape;

    UPROPERTY()
    TMap<FString, TObjectPtr<UTexture2D>> AtlasTexByShape;

    UPROPERTY()
    TMap<FString, FSignAtlasTable> AtlasTableByShape;

    UPROPERTY()
    TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> ISMByShape;
};
