// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "KillBarrierVolume.generated.h"

class UBoxComponent;
class ATempUIActor;
class UEntityBehaviorComponent;

/**
 * @brief Editor-placeable box marker that destroys any ATempUIActor whose position is pushed
 * inside it.
 *
 * This is a pure geometric marker, not a physics trigger volume — actors that are simulated
 * client-side (see UTollCameraDriveComponent) check their own next position against every
 * KillBarrierVolume in the world before moving there (via IsPointInside()), rather than relying
 * on UE's overlap system. That's deliberate: ATempUIActor's InteractionBox has collision set up
 * purely for hover/selection raycasts (all object-channel responses set to Ignore), so it would
 * never generate an overlap event with a trigger volume anyway.
 *
 * By default, any ATempUIActor entering the box is destroyed. Set TargetBehaviorClass in the
 * editor to scope it to only actors carrying a specific UEntityBehaviorComponent subclass, so a
 * barrier placed for one simulated entity type doesn't also affect unrelated real (server-driven)
 * entities that happen to be near the same spot.
 */
UCLASS()
class DT4MOB_API AKillBarrierVolume : public AActor
{
    GENERATED_BODY()

public:
    AKillBarrierVolume();

    /** @brief Box extent in Unreal units (cm), for placement/sizing in the editor. Purely a
     *  geometric marker — collision is disabled, see class comment. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* Box;

    /** @brief If set, only ATempUIActors whose BehaviorComponent is of this class are destroyed.
     *  Leave unset to destroy any ATempUIActor that enters the volume. */
    UPROPERTY(EditAnywhere, Category = "KillBarrier")
    TSubclassOf<UEntityBehaviorComponent> TargetBehaviorClass;

    /** @brief True if WorldLocation falls within this volume's box bounds (accounts for the
     *  actor's rotation, not just axis-aligned). */
    bool IsPointInside(const FVector& WorldLocation) const;

    /** @brief True if Entity is a type this barrier applies to, per TargetBehaviorClass. */
    bool AppliesTo(const ATempUIActor* Entity) const;
};
