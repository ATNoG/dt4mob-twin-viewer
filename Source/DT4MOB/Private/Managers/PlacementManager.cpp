#include "Managers/PlacementManager.h"
#include "Entities/IgnitionPointGhostActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

void UPlacementManager::EnterPlacementMode()
{
    if (bPlacing)
        return;

    bPlacing = true;

    UWorld* World = GetWorld();
    if (World)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        GhostActor = World->SpawnActor<AIgnitionPointGhostActor>(
            AIgnitionPointGhostActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (GhostActor)
            GhostActor->SetGhostVisible(false);
    }

    OnPlacementModeChanged.Broadcast(true);
}

void UPlacementManager::ConfirmPlacement(const FVector& WorldPos)
{
    if (!bPlacing)
        return;

    ExitPlacementMode();
    // The controller handles the actual PUT + spawn after calling this
    // by reading the position from its last hit result — nothing else needed here.
}

void UPlacementManager::CancelPlacement()
{
    if (!bPlacing)
        return;

    ExitPlacementMode();
}

void UPlacementManager::UpdateGhostPosition(const FVector& WorldPos, bool bHit)
{
    if (!GhostActor)
        return;

    GhostActor->SetGhostVisible(bHit);
    if (bHit)
        GhostActor->MoveToWorldPosition(WorldPos);
}

void UPlacementManager::ExitPlacementMode()
{
    bPlacing = false;

    if (GhostActor)
    {
        GhostActor->Destroy();
        GhostActor = nullptr;
    }

    OnPlacementModeChanged.Broadcast(false);
}
