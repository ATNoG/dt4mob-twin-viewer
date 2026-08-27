// Fill out your copyright notice in the Description page of Project Settings.

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
 * keyed by the sign's real attributes.Code — so a live sign renders with the correct real sign
 * face instead of whatever (possibly missing or wrong) model attributes.polygon points at.
 *
 * Unlike the benchmark's strategy, this stays one actor per sign (keeps hover/click/selection
 * working as normal) — it only replaces what mesh/texture that actor uses. The face is rendered
 * via a single-instance UInstancedStaticMeshComponent (not the generic mesh-layer path) because
 * M_SignAtlas's UV remap reads ISM per-instance custom data specifically — a regular
 * UStaticMeshComponent's SetCustomPrimitiveDataFloat does not feed the same material node, which
 * is why an earlier version of this class rendered the correct mesh but a blank/untextured face.
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
    // attributes.polygon GLB path runs instead — see OnEntityDataChanged().
    virtual bool HandlesOwnModelLoading() const override { return bUsingAtlasModel; }

private:
    /** @brief Same test-asset bucket the sign benchmark uses — only the 17 codes uploaded there
     *  (see SignBenchmark::CircleCodes()/TriangleCodes()) have real atlas art; any other real
     *  sign Code falls back to showing the whole atlas texture (visibly wrong, not silently). */
    static const FString &BaseUrl();

    /** @brief The same parent material the benchmark's AtlasMaterial strategy uses
     *  (/Game/Materials/SignBenchmark/M_SignAtlas) — TextureSampleParameter2D "AtlasTex",
     *  per-instance-custom-data-driven UV remap (ISM-specific, see class comment). */
    static UMaterialInterface *AtlasBaseMaterial();

    void LoadForCode(const FString &Code);
    void ApplyWhenReady();

    FString CurrentCode;
    bool bUsingAtlasModel = false;

    UPROPERTY()
    TObjectPtr<UStaticMesh> ShapeMesh;

    UPROPERTY()
    TObjectPtr<UTexture2D> AtlasTex;

    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> FaceISM;

    FSignAtlasTable AtlasTable;

    bool bMeshReady = false;
    bool bTexReady = false;
    bool bAtlasReady = false;

    /** @brief Guards ApplyWhenReady() against a stale, slower request landing after a newer
     *  LoadForCode() call already started (e.g. a fast Code correction right after spawn). */
    int32 RequestGeneration = 0;
};
