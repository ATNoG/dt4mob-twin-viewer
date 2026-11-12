// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UEntityBehaviorComponent, the base for per-actor behaviour that an entity
 *        type attaches to its ATempUIActor instances.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EntityBehaviorComponent.generated.h"

class ATempUIActor;

/**
 * @brief Base class for per-actor entity behaviour that needs live instance state (fetched
 *        GeoJSON, loaded model URLs, parsed geometry and so on).
 *
 * UEntityTypeExtension cannot hold this kind of data because one extension instance is
 * shared by every actor of a type. A type opts in by returning a subclass from
 * UEntityTypeExtension::GetBehaviorComponentClass(); ATempUIActor::Initialize() then creates
 * one instance on each actor and drives it through the virtual hooks below, without knowing
 * what the component does.
 *
 * Subclasses live under `Source/DT4MOB/Public/EntityDependencies/TYPE/`.
 *
 * @ingroup entitydeps
 */
UCLASS(Abstract)
class DT4MOB_API UEntityBehaviorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    /**
     * @brief Casts the component's owner to ATempUIActor.
     * @return The owning entity actor, or nullptr if the owner is missing or not an ATempUIActor.
     */
    ATempUIActor* GetOwnerEntity() const;

    /**
     * @brief Called once from ATempUIActor::Initialize(), after RawJson and StructInstance are
     *        filled and SetLocation() has run, and before the generic GLB model load.
     */
    virtual void OnEntityInitialized() {}

    /**
     * @brief Called after a live patch changes the owner's data.
     *
     * Fired from ATempUIActor::ApplyFullOrAttributePatch() (for "/" and attribute subtree
     * patches) and ATempUIActor::ApplyFeaturePropertiesPatch(). Single-value leaf patches and
     * per-entry feature patches do not call it.
     */
    virtual void OnEntityDataChanged() {}

    /**
     * @brief Whether ATempUIActor::TryLoadGlbModel() should skip its default single-URL GLB
     *        load (attributes.polygon) for this actor.
     *
     * Return true when the component loads its own model(s), typically from
     * OnEntityInitialized() or OnEntityDataChanged(). Checked every time TryLoadGlbModel() runs,
     * so the answer may change over the actor's lifetime.
     *
     * @return True if the component handles model loading; the default is false.
     */
    virtual bool HandlesOwnModelLoading() const { return false; }

    /**
     * @brief Supplies precise terrain-exclusion polygons in place of the mesh-derived default.
     *
     * Each entry is a named ring of lat/lon points (X = latitude, Y = longitude in degrees),
     * for example "Cone" and "Simulation" for fire. ATempUIActor's exclusion pass spawns one
     * polygon per entry with at least 3 points, so several can be active at once.
     *
     * @param OutPolygons  Map to fill with named rings. Left empty to use the default.
     * @return             True if at least one polygon was supplied. On false, the actor falls
     *                     back to a convex hull of its mesh (or its bounding box). The default
     *                     returns false.
     */
    virtual bool GetExclusionPolygons(TMap<FString, TArray<FVector2D>>& OutPolygons) const { return false; }

    /**
     * @brief Whether the owning ATempUIActor should cut Cesium terrain and photogrammetry out
     *        from under its model (see ATempUIActor::SpawnTerrainExclusionPolygon()).
     *
     * Only types whose models must sit flush on carved ground (fire cone and simulation)
     * should opt in. Signs, slope models and similar sit on top of the terrain as-is. Opting in
     * also stops the actor from tracing mesh hulls when layers load, since the shape is
     * expected to come from GetExclusionPolygons().
     *
     * @return True to request terrain exclusion; the default is false.
     */
    virtual bool WantsTerrainExclusion() const { return false; }
};
