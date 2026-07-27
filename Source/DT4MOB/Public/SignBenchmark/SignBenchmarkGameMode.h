// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "SignBenchmarkTypes.h"
#include "SignBenchmarkGameMode.generated.h"

class USignSpawnStrategy;
class ASignBenchmarkRoot;
class UMaterialInterface;

/**
 * @brief Console-command entry point for the sign-rendering thesis benchmark.
 * Standalone test harness, not part of the production Ditto entity pipeline —
 * set as a per-level GameMode Override on a dedicated empty test level, never
 * as the project's global default GameMode.
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

private:
    UPROPERTY()
    TObjectPtr<USignSpawnStrategy> Active;

    UPROPERTY()
    TObjectPtr<ASignBenchmarkRoot> Root;

    void ReportStats(const FString &Header);

    FString LastReport;
};
