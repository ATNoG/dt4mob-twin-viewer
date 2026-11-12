// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Enums, structs and helper functions shared by the road-sign rendering benchmark.
 *
 * The code tables and helpers here are also used outside the benchmark by
 * USinalizacaoModelComponent, which renders real sign entities with the AtlasMaterial assets.
 * @ingroup signbenchmark
 */

#pragma once

#include "CoreMinimal.h"
#include "SignBenchmarkTypes.generated.h"

class UStaticMesh;

/**
 * @brief Base shape of a road sign.
 *
 * Signs of the same shape share one mesh in the AtlasMaterial and PerTexture strategies,
 * and one atlas texture in AtlasMaterial.
 * @ingroup signbenchmark
 */
UENUM(BlueprintType)
enum class ESignShape : uint8
{
    Circle,  ///< Round sign: the C and D codes listed in SignBenchmark::CircleCodes().
    Triangle ///< Triangular sign: every other code, including the A codes in SignBenchmark::TriangleCodes().
};

/**
 * @brief Rendering strategy used by a benchmark run.
 *
 * Each value maps to one USignSpawnStrategy subclass, and to the asset folder of the same name
 * under the benchmark base URL.
 * @ingroup signbenchmark
 */
UENUM(BlueprintType)
enum class ESignStrategy : uint8
{
    PerSign,       ///< USignSpawnStrategy_PerSign: one GLB per code, one static mesh component per instance.
    AtlasMaterial, ///< USignSpawnStrategy_Atlas: one mesh, atlas texture and instanced component per shape.
    PerTexture     ///< USignSpawnStrategy_PerTexture: one mesh per shape, one texture and instanced component per code.
};

/**
 * @brief Rectangle of one sign's face inside its shape's atlas texture, in normalized 0 to 1 UV units.
 *
 * Parsed from one entry of the atlas JSON by USignAssetService. The atlas strategy passes the
 * four values in field order as per-instance custom data floats 0 to 3. The defaults cover the
 * whole atlas, which is also the fallback when a code has no entry.
 * @ingroup signbenchmark
 */
USTRUCT()
struct FSignAtlasUV
{
    GENERATED_BODY()

    /** @brief U offset of the cell, from the JSON field `u_offset`. */
    UPROPERTY()
    float UOffset = 0.0f;

    /** @brief V offset of the cell, from the JSON field `v_offset`. */
    UPROPERTY()
    float VOffset = 0.0f;

    /** @brief Width of the cell as a fraction of the atlas width, from the JSON field `u_scale`. */
    UPROPERTY()
    float UScale = 1.0f;

    /** @brief Height of the cell as a fraction of the atlas height, from the JSON field `v_scale`. */
    UPROPERTY()
    float VScale = 1.0f;
};

/**
 * @brief Code to atlas cell table for one shape, wrapped in a USTRUCT so the map can sit behind a single UPROPERTY.
 * @ingroup signbenchmark
 */
USTRUCT()
struct FSignAtlasTable
{
    GENERATED_BODY()

    /** @brief Atlas cell per sign code (for example "C1"), keyed by the top-level keys of the atlas JSON. */
    UPROPERTY()
    TMap<FString, FSignAtlasUV> Cells;
};

/**
 * @brief Row and column layout for the spawned sign instances.
 *
 * Instance i goes to column i % Columns and row i / Columns. Positions are relative to the
 * ASignBenchmarkRoot actor, which spawns at the world origin. Edited on ASignBenchmarkGameMode;
 * the SignBenchGrid console command changes Columns and the spacing at runtime.
 * @ingroup signbenchmark
 */
USTRUCT(BlueprintType)
struct FSignGridParams
{
    GENERATED_BODY()

    /** @brief Instances per row before wrapping to the next row. Values below 1 are treated as 1. */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    int32 Columns = 30;

    /** @brief Distance between columns along X, in Unreal units (cm). */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    float SpacingX = 400.0f;

    /** @brief Distance between rows along Y, in Unreal units (cm). */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    float SpacingY = 400.0f;

    /** @brief Location of instance 0, relative to the benchmark root actor. */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    FVector Origin = FVector::ZeroVector;

    /** @brief Yaw in degrees applied to every instance. Rotates each sign in place; the grid itself stays aligned to X and Y. */
    UPROPERTY(EditAnywhere, Category = "SignBenchmark")
    float Yaw = 0.0f;
};

/**
 * @brief Free helper functions and sign code tables for the benchmark.
 * @ingroup signbenchmark
 */
namespace SignBenchmark
{
    /**
     * @brief Computes the placement of the Index-th instance in the grid.
     *
     * @param Index   Zero-based position in the code sequence.
     * @param Params  Grid layout.
     * @return        Transform relative to the benchmark root: location
     *                Origin + (Col * SpacingX, Row * SpacingY, 0), yaw Params.Yaw, unit scale.
     */
    DT4MOB_API FTransform ComputeGridTransform(int32 Index, const FSignGridParams &Params);

    /**
     * @brief Returns the 10 circle sign codes with assets in the benchmark bucket.
     * @return Static list of C codes (prohibition) and D codes (obligation).
     */
    DT4MOB_API const TArray<FString> &CircleCodes();

    /**
     * @brief Returns the 7 triangle sign codes with assets in the benchmark bucket.
     * @return Static list of A codes (perigo, danger warning).
     */
    DT4MOB_API const TArray<FString> &TriangleCodes();

    /**
     * @brief Returns the shape a sign code uses.
     *
     * @param Code  Sign code, for example "C1" or "A12". Matching is case-sensitive.
     * @return      ESignShape::Circle if Code is in CircleCodes(), otherwise ESignShape::Triangle
     *              (including codes that are in neither list).
     */
    DT4MOB_API ESignShape ShapeForCode(const FString &Code);

    /**
     * @brief Returns the lowercase shape name used in asset file names and as the per-shape map key.
     *
     * @param Shape  Shape to name.
     * @return       "circle" or "triangle", as in `AtlasMaterial/circle.glb` or `triangle_atlas.png`.
     */
    DT4MOB_API FString ShapeName(ESignShape Shape);

    /**
     * @brief Builds a code sequence of TargetCount entries by cycling through all 17 codes.
     *
     * The order is the 10 circle codes followed by the 7 triangle codes, repeated, so each code
     * appears about TargetCount / 17 times (for example to approximate the 850 or so real signs).
     *
     * @param TargetCount  Number of entries to produce.
     * @return             The sequence; empty when TargetCount is 0 or negative.
     */
    DT4MOB_API TArray<FString> BuildCodeSequence(int32 TargetCount);

    /**
     * @brief Finds the material slot that shows the sign face.
     *
     * @param Mesh  Sign mesh to inspect. May be nullptr.
     * @return      Index of the first slot whose name contains "SignFace". Falls back to 1 when
     *              Mesh is nullptr, or when no slot matches (in that case a warning is logged).
     */
    DT4MOB_API int32 FindSignFaceSlot(const UStaticMesh *Mesh);

    /**
     * @brief Returns the display name of a strategy for logs and on-screen reports.
     *
     * @param Strategy  Strategy to name.
     * @return          "PerSign", "AtlasMaterial" or "PerTexture", or "Unknown" for any other value.
     */
    DT4MOB_API FString StrategyToString(ESignStrategy Strategy);

    /**
     * @brief Parses a strategy name typed as a console command argument.
     *
     * Accepts "PerSign", "AtlasMaterial" (or the short form "Atlas") and "PerTexture", ignoring case.
     *
     * @param In           Name to parse.
     * @param OutStrategy  Receives the parsed strategy. Left unchanged when parsing fails.
     * @return             True if In names a strategy, false otherwise.
     */
    DT4MOB_API bool ParseStrategy(const FString &In, ESignStrategy &OutStrategy);
}
