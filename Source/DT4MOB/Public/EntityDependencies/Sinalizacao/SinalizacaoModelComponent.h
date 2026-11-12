// Fill out your copyright notice in the Description page of Project Settings.
/**
 * @file
 * @brief Declares USinalizacaoModelComponent, which draws real road sign faces from the sign atlas.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityBehaviorComponent.h"
#include "SignBenchmark/SignBenchmarkTypes.h"
#include "SinalizacaoModelComponent.generated.h"

class UStaticMesh;
class UTexture2D;
class UMaterialInterface;
class UInstancedStaticMeshComponent;

/**
 * @brief Type-specific behavior for "sinalizacao" (road sign) entities: replaces the generic
 * per-entity GLB load (attributes.polygon) with the same known-good mesh + shared atlas texture
 * scheme validated by the SignBenchmark AtlasMaterial strategy (see Source/DT4MOB/Private/SignBenchmark/),
 * keyed by the sign's real attributes.Code: so a live sign renders with the correct real sign
 * face instead of whatever (possibly missing or wrong) model attributes.polygon points at.
 *
 * Unlike the benchmark's strategy, this stays one actor per sign (keeps hover/click/selection
 * working as normal): it only replaces what mesh/texture that actor uses. The face is rendered
 * via a single-instance UInstancedStaticMeshComponent (not the generic mesh-layer path) because
 * M_SignAtlas's UV remap reads ISM per-instance custom data specifically: a regular
 * UStaticMeshComponent's SetCustomPrimitiveDataFloat does not feed the same material node, which
 * is why an earlier version of this class rendered the correct mesh but a blank/untextured face.
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API USinalizacaoModelComponent : public UEntityBehaviorComponent
{
    GENERATED_BODY()

public:
    virtual void OnEntityInitialized() override;
    virtual void OnEntityDataChanged() override;

    // Only true once we've decided (synchronously, before ATempUIActor::TryLoadGlbModel() runs
    // in the same Initialize()/patch-handler call) that this sign's Code has real atlas art.
    // For a Code outside the benchmark's 17-entry catalog, this stays false and the generic
    // attributes.polygon GLB path runs instead: see OnEntityDataChanged().
    virtual bool HandlesOwnModelLoading() const override { return bUsingAtlasModel; }

private:
    /** @brief Same test-asset bucket the sign benchmark uses: only the 17 codes uploaded there
     *  (see SignBenchmark::CircleCodes()/TriangleCodes()) have real atlas art; any other real
     *  sign Code falls back to showing the whole atlas texture (visibly wrong, not silently).
     *  @return Base URL of the sign test assets. */
    static const FString &BaseUrl();

    /** @brief The same parent material the benchmark's AtlasMaterial strategy uses
     *  (/Game/Materials/SignBenchmark/M_SignAtlas): TextureSampleParameter2D "AtlasTex",
     *  per-instance-custom-data-driven UV remap (ISM-specific, see class comment).
     *  @return The material, or null if it could not be loaded. */
    static UMaterialInterface *AtlasBaseMaterial();

    /**
     * @brief Starts loading the shape mesh, the atlas texture and the atlas table for a sign code.
     * The three requests run in parallel; ApplyWhenReady() builds the sign once all have returned.
     * Results from an older call are ignored (see RequestGeneration).
     * @param Code Sign code from the entity data.
     */
    void LoadForCode(const FString &Code);

    /** @brief Builds the sign face once the mesh, texture and atlas table are all loaded; replaces any previous face. */
    void ApplyWhenReady();

    /** @brief Sign code currently shown. */
    FString CurrentCode;

    /** @brief True when CurrentCode has atlas art, so this component draws the sign instead of the generic GLB path. */
    bool bUsingAtlasModel = false;

    /** @brief Mesh for the sign's shape (circle, triangle, ...). */
    UPROPERTY()
    TObjectPtr<UStaticMesh> ShapeMesh;

    /** @brief Atlas texture with the art of every supported sign. */
    UPROPERTY()
    TObjectPtr<UTexture2D> AtlasTex;

    /** @brief Single-instance ISM that draws the sign face with the atlas material. */
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> FaceISM;

    /** @brief Atlas lookup table: where each sign code sits in AtlasTex. */
    FSignAtlasTable AtlasTable;

    bool bMeshReady = false;  ///< True once the shape mesh request has returned.
    bool bTexReady = false;   ///< True once the atlas texture request has returned.
    bool bAtlasReady = false; ///< True once the atlas table request has returned.

    /** @brief Guards ApplyWhenReady() against a stale, slower request landing after a newer
     *  LoadForCode() call already started (e.g. a fast Code correction right after spawn). */
    int32 RequestGeneration = 0;
};
