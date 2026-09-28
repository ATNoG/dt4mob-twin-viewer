// Fill out your copyright notice in the Description page of Project Settings.

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
 * @brief Console-command entry point for the sign-rendering thesis benchmark.
 * Standalone test harness, not part of the production Ditto entity pipeline —
 * set as a per-level GameMode Override on a dedicated empty test level, never
 * as the project's global default GameMode.
 *
 * Every SignBenchSpawn run appends one row to Saved/SignBenchmark/benchmark_results.csv,
 * written once its steady-state profiling window has closed (see below), so ten-run
 * averaging is a spreadsheet pivot on that file rather than hand-transcribed numbers.
 * That row covers what this harness measures directly (instance/component/material
 * counts, prepare/spawn time, StaticMesh_MB/Texture2D_MB summed via GetResourceSizeEx)
 * plus the exact time window (ProfilingWindowStart/End, ISO 8601) used for a matching
 * engine-stats capture: SettleDelaySeconds after spawn completes, this class runs
 * `csvprofile start` itself, waits CaptureWindowSeconds, then runs `csvprofile stop`.
 * That is Unreal's CSV Profiler (NOT `stat startfile`, which writes a binary .uestats
 * file readable only via Session Frontend) and writes an actual CSV under
 * Saved/Profiling/CSV/ — unit stats (frame/game/render/GPU time) and free memory are
 * captured by default; whether draw-call/triangle counts need an extra `-csvcategories`
 * category was not confirmed here and should be checked against the first real capture.
 * Match a capture to a benchmark_results.csv row by the profiling window timestamps,
 * since Unreal names that file by its own timestamp, not by strategy/count. This class
 * deliberately does not attempt to read engine frame-timing counters itself in C++,
 * since guessing at internal engine globals risks silently-wrong numbers; driving the
 * same console command the profiler itself uses is safer than reimplementing it.
 */
UCLASS()
class DT4MOB_API ASignBenchmarkGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    ASignBenchmarkGameMode();

    /** @brief Public test-asset bucket base URL. Not a secret, so unlike Ditto credentials this is a plain compiled-in default, editable per-level. */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    FString BaseUrl = TEXT("https://<bucket-host>/modelTest/");

    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    FSignGridParams Grid;

    /** @brief Parent material for the AtlasMaterial strategy. Must have a TextureSampleParameter2D named "AtlasTex" and 4 PerInstanceCustomData-driven UV remap nodes, and "Used with Instanced Static Meshes" checked. */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    TSoftObjectPtr<UMaterialInterface> AtlasBaseMaterial;

    /** @brief Parent material for the PerTexture strategy. Must have a TextureSampleParameter2D named "SignTex" and "Used with Instanced Static Meshes" checked. */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    TSoftObjectPtr<UMaterialInterface> TexturedBaseMaterial;

    /** @brief Clears any previous run and spawns Count sign instances (cycling the 17 available codes) using StrategyName ("PerSign" / "AtlasMaterial" / "PerTexture"). */
    UFUNCTION(Exec)
    void SignBenchSpawn(const FString &StrategyName, int32 Count);

    /** @brief Destroys the current run's root actor and releases the active strategy. */
    UFUNCTION(Exec)
    void SignBenchClear();

    /** @brief Re-displays the last run's stats without spawning anything. */
    UFUNCTION(Exec)
    void SignBenchInfo();

    /** @brief Changes the placement grid for subsequent SignBenchSpawn calls. */
    UFUNCTION(Exec)
    void SignBenchGrid(int32 Columns, float SpacingX, float SpacingY);

    /** @brief Changes the S3 base URL for subsequent SignBenchSpawn calls. */
    UFUNCTION(Exec)
    void SignBenchUrl(const FString &NewBaseUrl);

    /** @brief One-shot: deletes both on-disk asset caches (Saved/SignBenchCache and
     *  Saved/GlbCache, the latter shared with the production viewer) and marks the
     *  *next* SignBenchSpawn call as a cold-cache run, recorded in the CSV's
     *  CacheState column as "Cold". Reverts to "Warm" automatically after that one
     *  run — call this again before every deliberate cold repeat. */
    UFUNCTION(Exec)
    void SignBenchSetCold();

    /** @brief Delay after spawn completes before the engine-stats capture window opens,
     *  letting shader compilation / streaming settle so the window is steady-state only. */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    float SettleDelaySeconds = 1.0f;

    /** @brief Duration of the `stat startfile`/`stat stopfile` capture window itself. */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    float CaptureWindowSeconds = 3.0f;

private:
    UPROPERTY()
    TObjectPtr<USignSpawnStrategy> Active;

    UPROPERTY()
    TObjectPtr<ASignBenchmarkRoot> Root;

    void ReportStats(const FString &Header);
    void BeginEngineProfilingCapture();
    void EndEngineProfilingCapture();
    void AppendResultsCsvRow();

    FString LastReport;
    FString LastStrategyName;
    int32 LastRequestedCount = 0;
    FString LastCacheState;
    bool bNextRunIsCold = false;
    FString LastResultHeader;
    FDateTime ProfilingWindowStart;
    FDateTime ProfilingWindowEnd;

    FTimerHandle SettleTimer;
    FTimerHandle CaptureTimer;
};
