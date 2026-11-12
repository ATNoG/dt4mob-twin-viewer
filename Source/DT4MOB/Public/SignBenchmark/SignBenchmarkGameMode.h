// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Game mode that runs the road-sign rendering benchmark from console commands.
 * @ingroup signbenchmark
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SignBenchmarkTypes.h"
#include "TimerManager.h"
#include "SignBenchmarkGameMode.generated.h"

class USignSpawnStrategy;
class ASignBenchmarkRoot;
class UMaterialInterface;

/**
 * @brief Console command entry point for the sign rendering benchmark.
 *
 * Standalone test harness, not part of the Ditto entity pipeline. Set it as the GameMode
 * Override of a dedicated empty test level, never as the project's default game mode.
 *
 * A run (SignBenchSpawn) goes through these steps:
 * 1. The strategy loads its assets and spawns the instances; ReportStats() shows the counts and
 *    timings.
 * 2. After SettleDelaySeconds, BeginEngineProfilingCapture() runs `csvprofile start`.
 * 3. After CaptureWindowSeconds, EndEngineProfilingCapture() runs `csvprofile stop` and
 *    AppendResultsCsvRow() appends one row to Saved/SignBenchmark/benchmark_results.csv.
 *
 * Columns of benchmark_results.csv:
 * - Timestamp: when the row was written.
 * - Strategy: the strategy name exactly as typed in the command (not normalized, so "atlas" and
 *   "AtlasMaterial" end up as different values).
 * - RequestedCount: the Count argument.
 * - CacheState: "Cold" if SignBenchSetCold was run just before, otherwise "Warm".
 * - Result: "OK" or "FAILED", the success flag reported by the strategy.
 * - InstanceCount, ComponentCount, UniqueMeshCount, UniqueMaterialCount, UniqueTextureCount:
 *   from USignSpawnStrategy::FStats.
 * - PrepareMs, SpawnMs: FStats::PrepareSeconds and FStats::SpawnSeconds in milliseconds.
 * - StaticMesh_MB, Texture2D_MB: USignSpawnStrategy::GetAssetMemoryBytes() in MiB.
 * - ProfilingWindowStart, ProfilingWindowEnd: when `csvprofile start` and `csvprofile stop` ran.
 *
 * All timestamps come from FDateTime::Now() formatted with ToIso8601(). They are local time even
 * though the string ends in "Z".
 *
 * The CSV Profiler (not `stat startfile`, which writes a binary .uestats file that only Session
 * Frontend can read) writes one `Profile(*).csv` per capture under Saved/Profiling/CSV/, named by
 * its own timestamp. Tools/merge_benchmark_results.py matches each results row to the capture
 * whose timestamp falls inside its profiling window and averages the per-frame columns (frame,
 * game, render and GPU time, draw calls, primitives, memory). Unit timings and free memory are
 * captured by default; whether the draw call and primitive columns need an extra
 * `-csvcategories` argument was not confirmed here and should be checked against a real capture.
 * The class does not read engine frame timing counters itself, because guessing at internal
 * engine globals risks silently wrong numbers; it drives the same console command a person would.
 *
 * @note Let a run finish (the "SignBench: appended result row" log line) before starting another
 *       or calling SignBenchClear. Pending asset callbacks of the old strategy are not cancelled,
 *       and clearing during the capture window leaves `csvprofile` running.
 * @ingroup signbenchmark
 */
UCLASS()
class DT4MOB_API ASignBenchmarkGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    /** @brief Leaves DefaultPawnClass and PlayerControllerClass at the engine defaults (free-fly DefaultPawn). */
    ASignBenchmarkGameMode();

    /**
     * @brief Base URL of the public test-asset bucket, with a trailing slash.
     *
     * Each strategy appends its own folder (PerSign/, AtlasMaterial/ or PerTexture/). It is not a
     * secret, so unlike the Ditto credentials it is a plain compiled-in default, edited per level
     * or changed at runtime with SignBenchUrl.
     *
     * @note The compiled-in default contains a `<bucket-host>` placeholder and must be replaced
     *       before any asset can load.
     */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    FString BaseUrl = TEXT("https://<bucket-host>/modelTest/");

    /** @brief Placement grid for spawned instances. Edited on the game mode; SignBenchGrid changes Columns and spacing at runtime. */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    FSignGridParams Grid;

    /**
     * @brief Parent material for the AtlasMaterial strategy, loaded synchronously at the start of each AtlasMaterial run.
     *
     * Needs a texture parameter named "AtlasTex", a UV remap driven by PerInstanceCustomData 0 to 3
     * and "Used with Instanced Static Meshes" enabled. If it is unset the run fails.
     */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    TSoftObjectPtr<UMaterialInterface> AtlasBaseMaterial;

    /**
     * @brief Parent material for the PerTexture strategy, loaded synchronously at the start of each PerTexture run.
     *
     * Needs a texture parameter named "SignTex" and "Used with Instanced Static Meshes" enabled.
     * If it is unset the run fails.
     */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    TSoftObjectPtr<UMaterialInterface> TexturedBaseMaterial;

    /**
     * @brief Clears the previous run and spawns Count sign instances with the named strategy.
     *
     * Spawns a new ASignBenchmarkRoot, creates the strategy, consumes the cold-run flag set by
     * SignBenchSetCold and calls Execute(). The stats appear once the strategy completes, and the
     * CSV row is written after the profiling capture. An unknown strategy name shows an error and
     * leaves the previous run in place.
     *
     * @param StrategyName  "PerSign", "AtlasMaterial" (or "Atlas") or "PerTexture", ignoring case.
     *                      Written to the CSV Strategy column as typed.
     * @param Count         Number of instances, cycling through the 17 codes
     *                      (SignBenchmark::BuildCodeSequence()). 0 or less gives a FAILED run.
     */
    UFUNCTION(Exec)
    void SignBenchSpawn(const FString &StrategyName, int32 Count);

    /** @brief Cancels the settle and capture timers, clears the active strategy and destroys the root actor with every spawned component. */
    UFUNCTION(Exec)
    void SignBenchClear();

    /** @brief Shows the last run's stats again on screen and in the log, or "no run yet". */
    UFUNCTION(Exec)
    void SignBenchInfo();

    /**
     * @brief Changes the placement grid for later SignBenchSpawn calls. Origin and Yaw are not changed.
     *
     * @param Columns   Instances per row; values below 1 are stored as 1.
     * @param SpacingX  Distance between columns, in cm.
     * @param SpacingY  Distance between rows, in cm.
     */
    UFUNCTION(Exec)
    void SignBenchGrid(int32 Columns, float SpacingX, float SpacingY);

    /**
     * @brief Changes the asset bucket base URL for later SignBenchSpawn calls.
     * @param NewBaseUrl  New base URL. Must end with a slash, since strategies append folder names directly.
     */
    UFUNCTION(Exec)
    void SignBenchUrl(const FString &NewBaseUrl);

    /**
     * @brief Clears every asset cache and marks the next SignBenchSpawn as a cold-cache run.
     *
     * Empties the session caches of UGlbModelService and USignAssetService, then deletes
     * Saved/SignBenchCache and Saved/GlbCache. Saved/GlbCache is the production viewer's mesh
     * cache too; deleting it only means the viewer downloads the meshes again. The next run is
     * recorded as "Cold" in the CacheState column, and later runs go back to "Warm", so call this
     * again before each cold repeat. If a directory cannot be deleted, an error is shown and the
     * cold flag is not set (the session caches are already cleared by then).
     */
    UFUNCTION(Exec)
    void SignBenchSetCold();

    /**
     * @brief Seconds to wait after a run completes before the profiling capture starts.
     *
     * Gives shader compilation and streaming time to settle, so the capture covers steady state only.
     */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    float SettleDelaySeconds = 1.0f;

    /** @brief Length of the `csvprofile` capture window, in seconds. */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    float CaptureWindowSeconds = 3.0f;

private:
    /** @brief Strategy of the current run; nullptr after SignBenchClear. */
    UPROPERTY()
    TObjectPtr<USignSpawnStrategy> Active;

    /** @brief Actor that owns the current run's components; destroyed by SignBenchClear. */
    UPROPERTY()
    TObjectPtr<ASignBenchmarkRoot> Root;

    /**
     * @brief Shows the active strategy's stats on screen and in the log, keeps them for SignBenchInfo and starts the settle timer.
     *
     * Called from the strategy's completion callback on success and on failure.
     *
     * @param Header  "OK" or "FAILED"; shown in the report and written to the CSV Result column.
     */
    void ReportStats(const FString &Header);

    /** @brief Runs `csvprofile start`, records ProfilingWindowStart and starts the capture timer. Does nothing without GEngine. */
    void BeginEngineProfilingCapture();

    /** @brief Runs `csvprofile stop`, records ProfilingWindowEnd and appends the CSV row. */
    void EndEngineProfilingCapture();

    /** @brief Appends one row for the active run to Saved/SignBenchmark/benchmark_results.csv, writing the header row first if the file does not exist. */
    void AppendResultsCsvRow();

    /** @brief Multi-line stats text from the last ReportStats(), shown again by SignBenchInfo. */
    FString LastReport;

    /** @brief Strategy name as typed in the last SignBenchSpawn; CSV Strategy column. */
    FString LastStrategyName;

    /** @brief Count argument of the last SignBenchSpawn; CSV RequestedCount column. */
    int32 LastRequestedCount = 0;

    /** @brief "Cold" or "Warm" for the last run; CSV CacheState column. */
    FString LastCacheState;

    /** @brief Set by SignBenchSetCold, consumed and reset by the next SignBenchSpawn. */
    bool bNextRunIsCold = false;

    /** @brief "OK" or "FAILED" from the last ReportStats(); CSV Result column. */
    FString LastResultHeader;

    /** @brief Local time when `csvprofile start` ran; CSV ProfilingWindowStart column. */
    FDateTime ProfilingWindowStart;

    /** @brief Local time when `csvprofile stop` ran; CSV ProfilingWindowEnd column. */
    FDateTime ProfilingWindowEnd;

    /** @brief Fires BeginEngineProfilingCapture() SettleDelaySeconds after ReportStats(). */
    FTimerHandle SettleTimer;

    /** @brief Fires EndEngineProfilingCapture() CaptureWindowSeconds after the capture starts. */
    FTimerHandle CaptureTimer;
};
