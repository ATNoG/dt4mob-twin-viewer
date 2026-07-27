// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SignBenchmarkTypes.generated.h"

class UStaticMesh;

/** @brief Which base shape a sign uses — drives which shared mesh/atlas a code belongs to. */
UENUM(BlueprintType)
enum class ESignShape : uint8
{
    Circle,
    Triangle
};

/** @brief Which of the 3 thesis rendering strategies to spawn signs with. */
UENUM(BlueprintType)
enum class ESignStrategy : uint8
{
    PerSign,
    AtlasMaterial,
    PerTexture
};

/** @brief One sign's UV rect within its shape's packed atlas texture, 0..1 range. */
USTRUCT()
struct FSignAtlasUV
{
    GENERATED_BODY()

    UPROPERTY()
    float UOffset = 0.0f;

    UPROPERTY()
    float VOffset = 0.0f;

    UPROPERTY()
    float UScale = 1.0f;

    UPROPERTY()
    float VScale = 1.0f;
};

/** @brief GC-safe wrapper so a TMap<FString, FSignAtlasUV> can live behind a single UPROPERTY. */
USTRUCT()
struct FSignAtlasTable
{
    GENERATED_BODY()

    UPROPERTY()
    TMap<FString, FSignAtlasUV> Cells;
};

/** @brief Simple row/column grid layout for placing spawned sign instances. */
USTRUCT(BlueprintType)
struct FSignGridParams
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    int32 Columns = 30;

    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    float SpacingX = 400.0f;

    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    float SpacingY = 400.0f;

    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    FVector Origin = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    float Yaw = 0.0f;
};

namespace SignBenchmark
{
    /** @brief Row/column placement for the Index-th spawned instance. */
    DT4MOB_API FTransform ComputeGridTransform(int32 Index, const FSignGridParams &Params);

    /** @brief The 10 circle sign codes (prohibition C-codes + obligation D-codes) uploaded for the benchmark. */
    DT4MOB_API const TArray<FString> &CircleCodes();

    /** @brief The 7 triangle sign codes (perigo A-codes) uploaded for the benchmark. */
    DT4MOB_API const TArray<FString> &TriangleCodes();

    /** @brief Which shape a given code belongs to (circle or triangle code lists above). */
    DT4MOB_API ESignShape ShapeForCode(const FString &Code);

    /** @brief Lowercase shape name matching the S3 folder layout ("circle" / "triangle"). */
    DT4MOB_API FString ShapeName(ESignShape Shape);

    /** @brief Cycles all 17 codes interleaved until TargetCount entries are produced (e.g. to simulate ~850 real signs). */
    DT4MOB_API TArray<FString> BuildCodeSequence(int32 TargetCount);

    /** @brief Finds the material slot whose name contains "SignFace"; falls back to index 1 if not found (logs a warning either way it can't find one). */
    DT4MOB_API int32 FindSignFaceSlot(const UStaticMesh *Mesh);

    /** @brief "PerSign" / "AtlasMaterial" / "PerTexture" for on-screen/log reporting. */
    DT4MOB_API FString StrategyToString(ESignStrategy Strategy);

    /** @brief Case-insensitive parse of a strategy name from a console command argument. Returns false if unrecognized. */
    DT4MOB_API bool ParseStrategy(const FString &In, ESignStrategy &OutStrategy);
}
