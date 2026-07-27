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
     * @brief Optionally supplies one or more precise terrain-exclusion polygons, keyed by a
     *        logical name (e.g. "Cone" / "Simulation" for fire), each as world lat/lon points
     *        (X=lat, Y=lon). ATempUIActor::SpawnTerrainExclusionPolygon() spawns one polygon per
     *        entry, so multiple can be shown at once instead of being mutually exclusive.
     *        Return false (or leave OutPolygons empty) to fall back to the default
     *        convex-hull-from-visible-mesh-vertices behavior.
     */
    virtual bool GetExclusionPolygons(TMap<FString, TArray<FVector2D>>& OutPolygons) const { return false; }
};
