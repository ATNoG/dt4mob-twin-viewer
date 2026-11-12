// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief PerSign benchmark strategy: one self-contained GLB and one component per sign.
 * @ingroup signbenchmark
 */

#pragma once

#include "CoreMinimal.h"
#include "SignSpawnStrategy.h"
#include "SignSpawnStrategy_PerSign.generated.h"

class UStaticMesh;

/**
 * @brief Baseline strategy: one pre-baked GLB per sign code and one static mesh component per instance.
 *
 * Downloads `<BaseUrl>PerSign/<code>.glb` once for each distinct code in the sequence. Each GLB
 * holds the complete sign (geometry, material and face texture), so nothing is shared between
 * codes and no materials or textures are created at runtime. Every sequence entry gets its own
 * UStaticMeshComponent with Static mobility, attached to the root actor; no instancing is used.
 *
 * It is expected to lose to the other two strategies on download size (the geometry is
 * duplicated in every code's file) and on draw calls, which is why it is the reference point.
 *
 * Stats: ComponentCount equals InstanceCount, UniqueMeshCount is the number of codes loaded, and
 * UniqueMaterialCount and UniqueTextureCount are always 0.
 * @ingroup signbenchmark
 */
UCLASS()
class DT4MOB_API USignSpawnStrategy_PerSign : public USignSpawnStrategy
{
    GENERATED_BODY()

public:
    /**
     * @brief Requests one GLB per distinct code, then creates one UStaticMeshComponent per sequence entry.
     *
     * @param CodeSequence  Sign codes to place, one component per entry.
     * @param OnComplete    Called with true after spawning, or with false straight away when
     *                      CodeSequence is empty.
     */
    virtual void Execute(const TArray<FString> &CodeSequence, TFunction<void(bool)> OnComplete) override;

    /** @brief Empties MeshByCode, then drops the root reference. */
    virtual void Clear() override;

    /**
     * @brief Sums GetResourceSizeEx over the per-code meshes; the texture total is always 0.
     *
     * This strategy keeps no separate texture map: each code's face texture belongs to the
     * material loaded with its GLB, not to an asset tracked here.
     *
     * @note It was not verified whether UStaticMesh::GetResourceSizeEx includes the textures of the
     *       mesh's materials. Treat StaticMesh_MB for this strategy as a lower bound that may
     *       exclude texture cost, until it is checked against the engine's own memory report
     *       (`stat memory`, or Window > Statistics in the editor) for one run.
     *
     * @param OutStaticMeshBytes  Receives the summed size of the loaded meshes, in bytes.
     * @param OutTexture2DBytes   Always set to 0.
     */
    virtual void GetAssetMemoryBytes(int64 &OutStaticMeshBytes, int64 &OutTexture2DBytes) const override;

private:
    /** @brief Loaded mesh per sign code. Codes whose GLB failed to load are absent and their instances are skipped. */
    UPROPERTY()
    TMap<FString, TObjectPtr<UStaticMesh>> MeshByCode;
};
