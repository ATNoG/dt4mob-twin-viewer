// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SignBenchmarkTypes.h"
#include "SignSpawnStrategy.generated.h"

class ASignBenchmarkRoot;
class UGlbModelService;
class USignAssetService;
class USceneComponent;

/**
 * @brief Base class for one of the 3 thesis rendering strategies (PerSign,
 * AtlasMaterial, PerTexture). Holds async spawn state across many frames and
 * every GC-relevant asset/component it creates, so it's a UObject rather than a
 * plain struct or free function.
 */
UCLASS(Abstract)
class DT4MOB_API USignSpawnStrategy : public UObject
{
    GENERATED_BODY()

public:
    /** @brief Counts/timings reported after a run, for on-screen/log display and thesis sanity checks. */
    struct FStats
    {
        double PrepareSeconds = 0.0; // first request issued -> all assets ready
        double SpawnSeconds = 0.0;   // assets ready -> last instance placed
        int32 InstanceCount = 0;
        int32 ComponentCount = 0;
        int32 UniqueMeshCount = 0;
        int32 UniqueMaterialCount = 0;
        int32 UniqueTextureCount = 0;
    };

    void Init(ASignBenchmarkRoot *InRoot, const FString &InBaseUrl, const FSignGridParams &InGrid);

    /** @brief Fetches whatever assets this strategy needs and spawns one instance per entry in CodeSequence, then calls OnComplete(bSuccess). */
    virtual void Execute(const TArray<FString> &CodeSequence, TFunction<void(bool bSuccess)> OnComplete) PURE_VIRTUAL(USignSpawnStrategy::Execute, );

    /** @brief Releases this strategy's references so its components can be garbage collected once the owning root is destroyed. */
    virtual void Clear();

    const FStats &GetStats() const { return Stats; }

    /** @brief Sums GetResourceSizeEx over this strategy's unique mesh/texture assets, for the
     *  StaticMesh_MB/Texture2D_MB benchmark columns. Default no-op; each concrete strategy
     *  overrides it using whichever asset maps it actually tracks. */
    virtual void GetAssetMemoryBytes(int64 &OutStaticMeshBytes, int64 &OutTexture2DBytes) const
    {
        OutStaticMeshBytes = 0;
        OutTexture2DBytes = 0;
    }

protected:
    UGlbModelService *Glb() const;
    USignAssetService *Assets() const;

    /**
     * @brief Latch for "wait until N async requests have all completed". Call Arm()
     * before issuing any requests (so it tolerates a request resolving synchronously
     * from cache, mid-issue-loop), then Signal() once per request completion.
     */
    void Arm(int32 Count, TFunction<void()> OnAllReady);
    void Signal();

    UPROPERTY()
    TObjectPtr<ASignBenchmarkRoot> Root;

    FString BaseUrl;
    FSignGridParams Grid;
    FStats Stats;
    double TStart = 0.0;
    double TAssetsReady = 0.0;

private:
    int32 PendingCount = 0;
    bool bLatchFired = false;
    TFunction<void()> LatchCallback;
};
