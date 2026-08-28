// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "VehicleExtension.generated.h"

/** @brief Type-specific behavior for "traci" (vehicle) entities. */
UCLASS()
class DT4MOB_API UVehicleExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual TArray<FInfoField> GetDefaultInfoFields() const override;
    virtual FLinearColor GetBadgeColor() const override;
    virtual FString GetBadgeLabel(const FString& TypeKey) const override;
    virtual bool ShouldMonitorUpdateCadence() const override { return true; }

    // Position is read straight from RawJson (features.state.properties.lat/lon) by both
    // SetLocation() and the entity window, and vehicles have no behavior component, so the
    // per-message JsonObjectToUStruct reflection walk is dead work — skip it under load.
    virtual bool NeedsStructSyncOnLiveUpdate() const override { return false; }

    // TRACI stamps every vehicle in a run with one shared expiry_ts, so honoring it wipes the
    // whole fleet in a single frame when that deadline passes. Use a per-vehicle staleness
    // timeout instead — a vehicle is removed only if IT specifically stops updating for 60s.
    virtual float LiveStalenessTimeoutSeconds() const override { return 60.f; }

    // New vehicles can appear at any time while the camera sits still (the periodic tile
    // fetch only re-runs on tile/zoom change), so the live WS event is the only channel that
    // catches them — see UEntityTypeExtension::AllowsOnDemandSpawnFromWS().
    virtual bool AllowsOnDemandSpawnFromWS() const override { return true; }
};
