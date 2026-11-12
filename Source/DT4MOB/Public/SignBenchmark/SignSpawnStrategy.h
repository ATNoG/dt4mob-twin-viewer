// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Abstract base class for the sign benchmark rendering strategies.
 * @ingroup signbenchmark
 */

#pragma once

#include "CoreMinimal.h"
#include "SignBenchmarkTypes.h"
#include "SignSpawnStrategy.generated.h"

class ASignBenchmarkRoot;
class UGlbModelService;
class USignAssetService;
class USceneComponent;

/**
 * @brief Abstract base for one sign rendering strategy (PerSign, AtlasMaterial or PerTexture).
 *
 * A strategy downloads the assets it needs, waits for all of them with a countdown latch
 * (Arm() and Signal()), then places one sign instance per entry of a code sequence as
 * components of an ASignBenchmarkRoot, and records counts and timings in FStats.
 * ASignBenchmarkGameMode creates one strategy per SignBenchSpawn run, calls Init() and then
 * Execute(), and reads GetStats() and GetAssetMemoryBytes() for the results CSV.
 *
 * It is a UObject rather than a plain struct because its async state spans several frames and
 * the loaded meshes, textures and instanced components are kept in UPROPERTY maps so the
 * garbage collector does not free them during the run.
 *
 * @see USignSpawnStrategy_PerSign, USignSpawnStrategy_Atlas, USignSpawnStrategy_PerTexture
 * @ingroup signbenchmark
 */
UCLASS(Abstract)
class DT4MOB_API USignSpawnStrategy : public UObject
{
    GENERATED_BODY()

public:
    /**
     * @brief Counts and timings for one run, shown on screen and written to benchmark_results.csv.
     *
     * Every field stays 0 when Execute() fails before spawning. How the counts differ by strategy,
     * with all 17 codes present:
     *
     * | Field               | PerSign           | AtlasMaterial           | PerTexture             |
     * |---------------------|-------------------|-------------------------|------------------------|
     * | ComponentCount      | one per instance  | one ISM per shape (2)   | one ISM per code (17)  |
     * | UniqueMeshCount     | one per code (17) | one per shape (2)       | one per shape (2)      |
     * | UniqueMaterialCount | 0                 | one per shape (2)       | one per code (17)      |
     * | UniqueTextureCount  | 0                 | one atlas per shape (2) | one per code (17)      |
     *
     * PerSign reports 0 materials and textures because they come embedded in each GLB and are
     * not created or tracked by the strategy.
     */
    struct FStats
    {
        double PrepareSeconds = 0.0; ///< Seconds from Init() to the moment every asset request has completed: download or disk cache read, GLB parsing and PNG decoding. Written as PrepareMs.
        double SpawnSeconds = 0.0;   ///< Seconds from assets ready to the last instance placed: component creation and registration, material setup and AddInstance calls on the game thread. Written as SpawnMs.
        int32 InstanceCount = 0;       ///< Sign instances actually placed. Lower than the requested count when some assets failed to load.
        int32 ComponentCount = 0;      ///< Mesh components created (UStaticMeshComponent or UInstancedStaticMeshComponent).
        int32 UniqueMeshCount = 0;     ///< Distinct UStaticMesh assets loaded for the run.
        int32 UniqueMaterialCount = 0; ///< Material instance dynamics created at runtime.
        int32 UniqueTextureCount = 0;  ///< Textures decoded at runtime by USignAssetService.
    };

    /**
     * @brief Binds the strategy to a run, resets Stats and starts the PrepareSeconds clock.
     *
     * Call once, immediately before Execute().
     *
     * @param InRoot     Actor that owns every component the strategy spawns. Also used to reach
     *                   the world and the game instance subsystems.
     * @param InBaseUrl  Asset bucket base URL ending in a slash. Each strategy appends its own
     *                   folder, for example "PerSign/".
     * @param InGrid     Layout used to place the instances.
     */
    void Init(ASignBenchmarkRoot *InRoot, const FString &InBaseUrl, const FSignGridParams &InGrid);

    /**
     * @brief Downloads the assets this strategy needs, then spawns one instance per entry of CodeSequence.
     *
     * Requests are asynchronous unless they hit a session cache, in which case the whole run can
     * finish before this function returns. Spawning starts once every request has completed,
     * successfully or not. Codes or shapes whose assets failed to load are skipped with a
     * warning, and the run still reports success.
     *
     * @param CodeSequence  Sign codes to place, one instance per entry; entry i goes to grid
     *                      position i (see SignBenchmark::BuildCodeSequence()).
     * @param OnComplete    Called once: with true after spawning, or with false straight away
     *                      when there is nothing to spawn or a required base material is missing.
     */
    virtual void Execute(const TArray<FString> &CodeSequence, TFunction<void(bool bSuccess)> OnComplete) PURE_VIRTUAL(USignSpawnStrategy::Execute, );

    /**
     * @brief Drops the reference to the root actor; subclasses also empty their asset and component maps.
     *
     * Does not destroy components. The game mode destroys the root actor, which owns them.
     */
    virtual void Clear();

    /**
     * @brief Returns the counts and timings of the current run.
     * @return Stats filled in by Execute(); all zero until it has finished.
     */
    const FStats &GetStats() const { return Stats; }

    /**
     * @brief Reports the memory of this strategy's unique mesh and texture assets, for the StaticMesh_MB and Texture2D_MB CSV columns.
     *
     * Subclasses sum UObject::GetResourceSizeEx (EstimatedTotal mode) over the asset maps they
     * keep. This base version reports 0 for both.
     *
     * @param OutStaticMeshBytes  Receives the total size of the loaded meshes, in bytes.
     * @param OutTexture2DBytes   Receives the total size of the runtime-decoded textures, in bytes.
     */
    virtual void GetAssetMemoryBytes(int64 &OutStaticMeshBytes, int64 &OutTexture2DBytes) const
    {
        OutStaticMeshBytes = 0;
        OutTexture2DBytes = 0;
    }

protected:
    /**
     * @brief Looks up the GLB mesh service.
     * @return The game instance's UGlbModelService, or nullptr when Root is unset or has no game instance.
     */
    UGlbModelService *Glb() const;

    /**
     * @brief Looks up the texture and atlas JSON service.
     * @return The game instance's USignAssetService, or nullptr when Root is unset or has no game instance.
     */
    USignAssetService *Assets() const;

    /**
     * @brief Arms a countdown latch that runs OnAllReady after Count calls to Signal().
     *
     * Call before issuing any request, so a request that completes synchronously from a cache
     * while the issue loop is still running is counted correctly. With Count of 0 or less the
     * callback runs immediately.
     *
     * @param Count       Number of Signal() calls to wait for.
     * @param OnAllReady  Runs once, from inside the last Signal() call (or from Arm() itself).
     */
    void Arm(int32 Count, TFunction<void()> OnAllReady);

    /**
     * @brief Marks one request as completed and runs the armed callback when none are left.
     *
     * Calls after the callback has run are ignored.
     */
    void Signal();

    /** @brief Actor that owns this run's components. Set by Init(), cleared by Clear(). */
    UPROPERTY()
    TObjectPtr<ASignBenchmarkRoot> Root;

    /** @brief Asset bucket base URL from Init(), with a trailing slash. */
    FString BaseUrl;

    /** @brief Placement grid from Init(). */
    FSignGridParams Grid;

    /** @brief Counts and timings of the current run. Reset by Init(). */
    FStats Stats;

    /** @brief FPlatformTime::Seconds() at Init(): start of the PrepareSeconds interval. */
    double TStart = 0.0;

    /** @brief FPlatformTime::Seconds() when the latch fired: end of the PrepareSeconds interval. */
    double TAssetsReady = 0.0;

private:
    /** @brief Signal() calls still expected before the latch fires. */
    int32 PendingCount = 0;

    /** @brief True once the latch callback has run. */
    bool bLatchFired = false;

    /** @brief Callback passed to Arm(). */
    TFunction<void()> LatchCallback;
};
