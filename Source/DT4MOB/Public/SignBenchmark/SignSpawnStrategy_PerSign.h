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

    /** @brief This strategy does not keep a separate UTexture2D map (its per-code texture is
     *  baked into the mesh's own material, not tracked here as a distinct asset), so
     *  OutTexture2DBytes is always 0. Whether that texture's memory is counted at all
     *  depends on whether UStaticMesh::GetResourceSizeEx includes its assigned material's
     *  textures, which was not verified here — treat StaticMesh_MB for this strategy as a
     *  lower bound, not confirmed to include texture cost, until checked against the editor's
     *  own memory report (Window > Statistics, or `stat memory`) for one run. */
    virtual void GetAssetMemoryBytes(int64 &OutStaticMeshBytes, int64 &OutTexture2DBytes) const override;

private:
    UPROPERTY()
    TMap<FString, TObjectPtr<UStaticMesh>> MeshByCode;
};
