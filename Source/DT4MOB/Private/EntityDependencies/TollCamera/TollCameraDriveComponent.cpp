// Fill out your copyright notice in the Description page of Project Settings.

/** @file TollCameraDriveComponent.cpp
 *  @brief Implementation of UTollCameraDriveComponent. All logic documentation is in the header.
 */
#include "EntityDependencies/TollCamera/TollCameraDriveComponent.h"
#include "Entities/TempUIActor.h"
#include "EntityStructs/TollCameraStruct.h"
#include "Gameplay/KillBarrierVolume.h"
#include "Services/CoordinatesConversionService.h"
#include "TimerManager.h"
#include "EngineUtils.h"

void UTollCameraDriveComponent::OnEntityInitialized()
{
    OnEntityDataChanged();
}

void UTollCameraDriveComponent::OnEntityDataChanged()
{
    ATempUIActor* Owner = GetOwnerEntity();
    if (!Owner || !Owner->StructInstance.IsValid())
        return;

    const FTollCameraData* Data =
        reinterpret_cast<const FTollCameraData*>(Owner->StructInstance->GetStructMemory());

    const FString& Timestamp = Data->features.State.properties.timeOfMeasurement;
    if (Timestamp.IsEmpty() || Timestamp == LastDrivenTimestamp)
        return;

    LastDrivenTimestamp = Timestamp;
    StartDrive();
}

void UTollCameraDriveComponent::StartDrive()
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    ElapsedDriveSeconds = 0.0;
    World->GetTimerManager().ClearTimer(DriveTimer);
    World->GetTimerManager().SetTimer(
        DriveTimer, this, &UTollCameraDriveComponent::AdvanceStep, WaypointIntervalSeconds, true);
}

void UTollCameraDriveComponent::AdvanceStep()
{
    ElapsedDriveSeconds += WaypointIntervalSeconds;

    ATempUIActor* Owner = GetOwnerEntity();
    UWorld* World = GetWorld();
    if (!Owner || !World || ElapsedDriveSeconds >= MaxDriveSeconds)
    {
        if (World)
            World->GetTimerManager().ClearTimer(DriveTimer);
        return;
    }

    const double CurLat = Owner->GetTargetLatitude();
    const double CurLon = Owner->GetTargetLongitude();

    // Advance one step along RoadHeadingDeg (0 = north, 90 = east, clockwise) at CruiseSpeedKmh.
    const double DistanceMeters = (CruiseSpeedKmh / 3.6) * WaypointIntervalSeconds;
    const double HeadingRad = FMath::DegreesToRadians(RoadHeadingDeg);
    const double DNorth = DistanceMeters * FMath::Cos(HeadingRad);
    const double DEast  = DistanceMeters * FMath::Sin(HeadingRad);

    constexpr double MetersPerDegLat = 111320.0;
    const double NewLat = CurLat + DNorth / MetersPerDegLat;
    const double NewLon = CurLon + DEast / (MetersPerDegLat * FMath::Cos(FMath::DegreesToRadians(CurLat)));

    // Stop and despawn instead of moving there if the next step would land inside a placed
    // kill barrier — see AKillBarrierVolume. Primary despawn mechanism; MaxDriveSeconds above
    // is only a backstop in case a barrier is missing/misplaced.
    //
    // Z uses the actor's current (ground-snapped) height rather than a raw ellipsoid-altitude
    // conversion at 0.0 — WGS-84 ellipsoid height and real terrain height can differ by tens of
    // metres here (geoid offset), which was enough to push the point outside the box's Z extent
    // even when X/Y lined up exactly over the barrier.
    FVector NewWorldPos = UCoordinatesConversionService::Get()->ConvertWSG84ToUELocal(NewLat, NewLon, 0.0);
    NewWorldPos.Z = Owner->GetActorLocation().Z;

    int32 BarrierCount = 0;
    for (TActorIterator<AKillBarrierVolume> It(World); It; ++It)
    {
        ++BarrierCount;
        const bool bApplies = It->AppliesTo(Owner);
        const FVector LocalPt = It->Box->GetComponentTransform().InverseTransformPosition(NewWorldPos);
        const FVector Extent = It->Box->GetUnscaledBoxExtent();
        UE_LOG(LogTemp, Log, TEXT("TollCameraDrive [%s]: barrier '%s' applies=%d localPt=(%.0f,%.0f,%.0f) extent=(%.0f,%.0f,%.0f)"),
            *Owner->GetThingId(), *It->GetActorNameOrLabel(), bApplies ? 1 : 0,
            LocalPt.X, LocalPt.Y, LocalPt.Z, Extent.X, Extent.Y, Extent.Z);

        if (bApplies && It->IsPointInside(NewWorldPos))
        {
            UE_LOG(LogTemp, Log, TEXT("TollCameraDrive [%s]: destroyed by barrier '%s'"),
                *Owner->GetThingId(), *It->GetActorNameOrLabel());
            World->GetTimerManager().ClearTimer(DriveTimer);
            Owner->Destroy();
            return;
        }
    }
    if (BarrierCount == 0 && !bWarnedNoBarriers)
    {
        bWarnedNoBarriers = true;
        UE_LOG(LogTemp, Warning, TEXT("TollCameraDrive [%s]: no AKillBarrierVolume found in the world"), *Owner->GetThingId());
    }

    Owner->SetSyntheticMovementTarget(NewLat, NewLon, CruiseSpeedKmh, RoadHeadingDeg, false);
}
