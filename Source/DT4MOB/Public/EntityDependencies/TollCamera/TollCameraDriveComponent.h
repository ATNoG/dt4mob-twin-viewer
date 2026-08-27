// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityBehaviorComponent.h"
#include "TollCameraDriveComponent.generated.h"

/**
 * @brief Simulates continuous road motion for toll camera / LiDAR detection entities.
 *
 * Each detection is a single, near-instantaneous sighting of a real (and usually different)
 * vehicle — the sensor doesn't report a trajectory, and consecutive detections on the same
 * thingId arrive too rarely to derive a heading from position deltas. So instead of trying to
 * follow the real (jittery, sensor-cadence-paced) updates, this component treats every new
 * detection as a spawn point and drives the actor forward at a fixed synthetic cruise speed
 * along a fixed road heading, by periodically pushing synthetic waypoints via
 * ATempUIActor::SetSyntheticMovementTarget() — reusing the existing smooth lerp/rotation/
 * ground-snap machinery built for TRACI vehicles.
 *
 * Bounded by MaxDriveSeconds as a backstop, but the primary despawn mechanism is expected to be
 * a placed AKillBarrierVolume before the road curve (see Gameplay/KillBarrierVolume.h) — the
 * heading/speed here are not tied to real road geometry, so letting a synthetic drive run
 * indefinitely would eventually send the box off the visible road.
 */
UCLASS()
class DT4MOB_API UTollCameraDriveComponent : public UEntityBehaviorComponent
{
    GENERATED_BODY()

public:
    virtual void OnEntityInitialized() override;
    virtual void OnEntityDataChanged() override;

    /** @brief Compass heading (0 = north, 90 = east, clockwise) of the road at this camera site.
     *  Fixed rather than derived — see class comment. Tune per deployment/site in the editor. */
    UPROPERTY(EditAnywhere, Category = "TollCamera")
    double RoadHeadingDeg = 236.0;

    /** @brief Synthetic cruise speed in km/h — chosen to read as a car, not tied to the sensor's
     *  reported speedKmh (which is per-vehicle and would look erratic frame to frame). */
    UPROPERTY(EditAnywhere, Category = "TollCamera")
    double CruiseSpeedKmh = 45.0;

    /** @brief How often a new synthetic waypoint is pushed. Smaller = smoother motion, at the
     *  cost of more SetSyntheticMovementTarget calls. */
    UPROPERTY(EditAnywhere, Category = "TollCamera")
    double WaypointIntervalSeconds = 0.25;

    /** @brief Backstop lifetime for the synthetic drive — stops pushing new waypoints after this
     *  long even if nothing else despawns the actor. Tune so it comfortably clears before the
     *  actor would otherwise reach a road curve if the kill barrier is ever missing/misplaced. */
    UPROPERTY(EditAnywhere, Category = "TollCamera")
    double MaxDriveSeconds = 8.0;

private:
    /** @brief Starts (or restarts) the synthetic drive from the actor's current target position. */
    void StartDrive();

    /** @brief Timer callback: advances the actor one WaypointIntervalSeconds step along RoadHeadingDeg. */
    void AdvanceStep();

    FTimerHandle DriveTimer;
    double ElapsedDriveSeconds = 0.0;

    /** @brief timeOfMeasurement of the last detection a drive was started for — guards against
     *  restarting the drive on unrelated patches (e.g. attribute-only updates). */
    FString LastDrivenTimestamp;

    /** @brief Set once an AdvanceStep() call finds zero AKillBarrierVolume actors in the world,
     *  so the "no barrier found" warning only logs once per actor instead of spamming. */
    bool bWarnedNoBarriers = false;
};
