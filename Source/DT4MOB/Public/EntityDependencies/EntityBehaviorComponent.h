// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EntityBehaviorComponent.generated.h"

class ATempUIActor;

/**
 * @brief Base class for optional, per-actor entity behavior that needs live instance state
 *        (fetched GeoJSON, loaded model URLs, parsed geometry, ...) — unlike
 *        UEntityTypeExtension, which is a single shared instance per type and therefore
 *        cannot hold per-entity data.
 *
 * A type opts in by returning a subclass from its UEntityTypeExtension::GetBehaviorComponentClass().
 * ATempUIActor::Initialize() then attaches one instance of that class to the actor generically —
 * the actor has no per-type knowledge of what the component does.
 *
 * Register a subclass under Source/DT4MOB/Public/EntityDependencies/<TypeName>/.
 */
UCLASS(Abstract)
class DT4MOB_API UEntityBehaviorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    /** @brief Returns the owning ATempUIActor, or nullptr if not yet attached to one. */
    ATempUIActor* GetOwnerEntity() const;

    /** @brief Called once, right after ATempUIActor::Initialize() has populated RawJson/StructInstance. */
    virtual void OnEntityInitialized() {}

    /** @brief Called after every live patch to the owner's RawJson (see ATempUIActor::OnEntityDataChanged). */
    virtual void OnEntityDataChanged() {}

    /**
     * @brief If true, ATempUIActor::TryLoadGlbModel() skips its default single-URL GLB load —
     *        the component is expected to handle its own model loading (e.g. from
     *        OnEntityInitialized/OnEntityDataChanged) instead. Default: false.
     */
    virtual bool HandlesOwnModelLoading() const { return false; }

    /**
     * @brief Optionally supplies precise terrain-exclusion polygon points (world lat/lon, X=lat,
     *        Y=lon), used by ATempUIActor::SpawnTerrainExclusionPolygon() in place of its default
     *        convex-hull-from-mesh-vertices fallback. Return false to use the default.
     */
    virtual bool GetExclusionPolygonPoints(TArray<FVector2D>& OutPoints) const { return false; }
};
