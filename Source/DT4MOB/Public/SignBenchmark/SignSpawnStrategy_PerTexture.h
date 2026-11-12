// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief PerTexture benchmark strategy: shared mesh per shape, one texture, material and instanced component per code.
 * @ingroup signbenchmark
 */

#pragma once

#include "CoreMinimal.h"
#include "SignSpawnStrategy.h"
#include "SignSpawnStrategy_PerTexture.generated.h"

class UStaticMesh;
class UTexture2D;
class UMaterialInterface;
class UInstancedStaticMeshComponent;

/**
 * @brief Per-texture strategy: meshes are shared per shape, but each sign code gets its own texture, material and instanced component.
 *
 * Downloads `<BaseUrl>PerTexture/<shape>.glb` once per shape present in the sequence and
 * `<BaseUrl>PerTexture/<code>.png` once per distinct code. For each code it creates a
 * UInstancedStaticMeshComponent using its shape's mesh, with one material instance dynamic of
 * TexturedBaseMaterial (its "SignTex" parameter set to the code's texture) on the sign face slot.
 * Each sequence entry becomes one instance of its code's component. No per-instance custom data
 * is used.
 *
 * An ISM has one material per slot, so instances with different textures cannot share a
 * component. Compared with AtlasMaterial this gives the same meshes and instance count, but one
 * component, material and texture per distinct code (17 when all codes are present) instead of
 * one per shape.
 * @ingroup signbenchmark
 */
UCLASS()
class DT4MOB_API USignSpawnStrategy_PerTexture : public USignSpawnStrategy
{
    GENERATED_BODY()

public:
    /**
     * @brief Parent material for the per-code material instances.
     *
     * Needs a texture parameter named "SignTex" and "Used with Instanced Static Meshes" enabled.
     * ASignBenchmarkGameMode sets it from its own TexturedBaseMaterial property before Execute().
     */
    UPROPERTY()
    TObjectPtr<UMaterialInterface> TexturedBaseMaterial;

    /**
     * @brief Loads one mesh per shape and one texture per distinct code, then builds one ISM per code.
     *
     * A code whose texture or shape mesh failed to load is skipped with a warning, and so are all
     * of its instances.
     *
     * @param CodeSequence  Sign codes to place, one instance per entry.
     * @param OnComplete    Called with true after spawning, or with false straight away when
     *                      TexturedBaseMaterial is not set or CodeSequence is empty.
     */
    virtual void Execute(const TArray<FString> &CodeSequence, TFunction<void(bool)> OnComplete) override;

    /** @brief Empties the mesh, texture and ISM maps, then drops the root reference. */
    virtual void Clear() override;

    /**
     * @brief Sums GetResourceSizeEx over the per-shape meshes and per-code textures.
     *
     * @param OutStaticMeshBytes  Receives the summed size of the shape meshes, in bytes.
     * @param OutTexture2DBytes   Receives the summed size of the decoded sign textures, in bytes.
     */
    virtual void GetAssetMemoryBytes(int64 &OutStaticMeshBytes, int64 &OutTexture2DBytes) const override;

private:
    /** @brief Shared sign mesh per shape name ("circle" or "triangle"). */
    UPROPERTY()
    TMap<FString, TObjectPtr<UStaticMesh>> MeshByShape;

    /** @brief Decoded face texture per sign code. */
    UPROPERTY()
    TMap<FString, TObjectPtr<UTexture2D>> TexByCode;

    /** @brief The instanced component created for each sign code. */
    UPROPERTY()
    TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> ISMByCode;
};
