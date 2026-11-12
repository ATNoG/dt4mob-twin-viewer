// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief AtlasMaterial benchmark strategy: one mesh, atlas texture and instanced component per shape.
 * @ingroup signbenchmark
 */

#pragma once

#include "CoreMinimal.h"
#include "SignSpawnStrategy.h"
#include "SignSpawnStrategy_Atlas.generated.h"

class UStaticMesh;
class UTexture2D;
class UMaterialInterface;
class UInstancedStaticMeshComponent;

/**
 * @brief Atlas strategy: every sign of a shape is one instance of a single instanced component that samples a shared atlas.
 *
 * For each shape present in the sequence it downloads three files from `<BaseUrl>AtlasMaterial/`:
 * - `<shape>.glb`: the sign mesh shared by every code of that shape.
 * - `<shape>_atlas.png`: the faces of every code of that shape packed into one texture.
 * - `<shape>_atlas.json`: the cell of each code in that texture (see FSignAtlasTable).
 *
 * It then creates one UInstancedStaticMeshComponent per shape, with one material instance dynamic
 * of AtlasBaseMaterial (its "AtlasTex" parameter set to the shape's atlas) on the mesh's sign face
 * slot. The other slots keep the materials loaded with the GLB. Each sequence entry becomes one
 * instance, and four per-instance custom data floats (u_offset, v_offset, u_scale, v_scale) pick
 * the atlas cell it shows. A code missing from the atlas JSON falls back to the whole atlas, so
 * the error is visible.
 *
 * With both shapes present this is 2 components, 2 meshes, 2 materials and 2 textures,
 * independent of the instance count.
 * @ingroup signbenchmark
 */
UCLASS()
class DT4MOB_API USignSpawnStrategy_Atlas : public USignSpawnStrategy
{
    GENERATED_BODY()

public:
    /**
     * @brief Parent material for the per-shape material instances.
     *
     * Needs a texture parameter named "AtlasTex", a UV remap driven by PerInstanceCustomData 0 to 3
     * (u_offset, v_offset, u_scale, v_scale) and "Used with Instanced Static Meshes" enabled.
     * ASignBenchmarkGameMode sets it from its own AtlasBaseMaterial property before Execute().
     */
    UPROPERTY()
    TObjectPtr<UMaterialInterface> AtlasBaseMaterial;

    /**
     * @brief Loads the mesh, atlas texture and atlas JSON of every shape in the sequence, then builds one ISM per shape.
     *
     * Three requests are issued per shape. A shape with any missing asset is skipped with a
     * warning, and so are all of its instances. The render state of each ISM is marked dirty once
     * after all instances are added, not once per instance.
     *
     * @param CodeSequence  Sign codes to place, one instance per entry.
     * @param OnComplete    Called with true after spawning, or with false straight away when
     *                      AtlasBaseMaterial is not set or CodeSequence is empty.
     */
    virtual void Execute(const TArray<FString> &CodeSequence, TFunction<void(bool)> OnComplete) override;

    /** @brief Empties the mesh, texture, atlas table and ISM maps, then drops the root reference. */
    virtual void Clear() override;

    /**
     * @brief Sums GetResourceSizeEx over the per-shape meshes and atlas textures.
     *
     * @param OutStaticMeshBytes  Receives the summed size of the shape meshes, in bytes.
     * @param OutTexture2DBytes   Receives the summed size of the decoded atlas textures, in bytes.
     */
    virtual void GetAssetMemoryBytes(int64 &OutStaticMeshBytes, int64 &OutTexture2DBytes) const override;

private:
    /** @brief Shared sign mesh per shape name ("circle" or "triangle"). */
    UPROPERTY()
    TMap<FString, TObjectPtr<UStaticMesh>> MeshByShape;

    /** @brief Decoded atlas texture per shape name. */
    UPROPERTY()
    TMap<FString, TObjectPtr<UTexture2D>> AtlasTexByShape;

    /** @brief Code to atlas cell table per shape name, copied from USignAssetService's cache. */
    UPROPERTY()
    TMap<FString, FSignAtlasTable> AtlasTableByShape;

    /** @brief The instanced component created for each shape name. */
    UPROPERTY()
    TMap<FString, TObjectPtr<UInstancedStaticMeshComponent>> ISMByShape;
};
