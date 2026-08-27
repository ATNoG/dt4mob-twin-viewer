// Fill out your copyright notice in the Description page of Project Settings.

/** @file KillBarrierVolume.cpp
 *  @brief Implementation of AKillBarrierVolume. All logic documentation is in the header.
 */
#include "Gameplay/KillBarrierVolume.h"
#include "Components/BoxComponent.h"
#include "Entities/TempUIActor.h"
#include "EntityDependencies/EntityBehaviorComponent.h"

AKillBarrierVolume::AKillBarrierVolume()
{
    PrimaryActorTick.bCanEverTick = false;

    Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
    RootComponent = Box;

    Box->SetBoxExtent(FVector(500.f, 500.f, 500.f));
    Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

bool AKillBarrierVolume::IsPointInside(const FVector& WorldLocation) const
{
    const FVector LocalPoint = Box->GetComponentTransform().InverseTransformPosition(WorldLocation);
    const FVector Extent = Box->GetUnscaledBoxExtent();
    return FMath::Abs(LocalPoint.X) <= Extent.X
        && FMath::Abs(LocalPoint.Y) <= Extent.Y
        && FMath::Abs(LocalPoint.Z) <= Extent.Z;
}

bool AKillBarrierVolume::AppliesTo(const ATempUIActor* Entity) const
{
    if (!Entity)
        return false;

    if (!TargetBehaviorClass)
        return true;

    return Entity->BehaviorComponent && Entity->BehaviorComponent->IsA(TargetBehaviorClass);
}
