// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares AKillBarrierVolume, a box marker that despawns client-simulated entities.
 * @ingroup gameplay
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KillBarrierVolume.generated.h"

class UBoxComponent;
class ATempUIActor;
class UEntityBehaviorComponent;

/**
 * @brief Editor-placed box that marks where client-simulated entities should be despawned.
 *
 * The volume does not destroy anything by itself and is not a physics trigger. Actors that
 * are simulated client-side (currently UTollCameraDriveComponent) iterate every
 * AKillBarrierVolume in the world before each step, call AppliesTo() and IsPointInside() on
 * their next position, and destroy their owner when both return true. The overlap system is
 * avoided on purpose: ATempUIActor's InteractionBox only has collision for hover/selection
 * traces (all object-channel responses set to Ignore), so it would never produce an overlap
 * event with a trigger volume.
 *
 * With TargetBehaviorClass unset the barrier applies to every ATempUIActor. Set it in the
 * editor to limit the barrier to actors that carry a specific UEntityBehaviorComponent
 * subclass, so a barrier placed for one simulated entity type does not affect unrelated
 * server-driven entities near the same spot.
 *
 * @ingroup gameplay
 */
UCLASS()
class DT4MOB_API AKillBarrierVolume : public AActor
{
    GENERATED_BODY()

public:
    /** @brief Creates the root box (500 cm half-extent on each axis, collision disabled) and disables ticking. */
    AKillBarrierVolume();

    /**
     * @brief Root box component that defines the barrier region; size and place it in the editor.
     *
     * Extent is in Unreal units (cm). Collision is disabled because the box is only a geometric
     * marker (see the class comment).
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* Box;

    /**
     * @brief Behavior component class this barrier is limited to. Set per instance in the editor.
     *
     * When set, AppliesTo() only accepts ATempUIActors whose BehaviorComponent is of this class
     * (or a subclass). Leave unset to apply to every ATempUIActor.
     */
    UPROPERTY(EditAnywhere, Category = "KillBarrier")
    TSubclassOf<UEntityBehaviorComponent> TargetBehaviorClass;

    /**
     * @brief Tests whether a world-space point lies inside the box.
     *
     * The point is transformed into the box component's local space, so actor rotation and
     * scale are taken into account. Points exactly on a face count as inside.
     *
     * @param WorldLocation  Point to test, in Unreal world space.
     * @return True if the point is inside or on the box.
     */
    bool IsPointInside(const FVector& WorldLocation) const;

    /**
     * @brief Checks whether this barrier should despawn the given entity.
     * @param Entity  Entity actor to test; may be nullptr.
     * @return False for nullptr; true if TargetBehaviorClass is unset; otherwise true only when
     *         the entity's BehaviorComponent is an instance of TargetBehaviorClass.
     */
    bool AppliesTo(const ATempUIActor* Entity) const;
};
