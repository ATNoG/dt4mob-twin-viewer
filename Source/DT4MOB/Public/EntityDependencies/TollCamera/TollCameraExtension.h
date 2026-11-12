// Fill out your copyright notice in the Description page of Project Settings.
/**
 * @file
 * @brief Declares UTollCameraExtension, the type hooks for toll camera and LiDAR detection things.

 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "EntityDependencies/TollCamera/TollCameraDriveComponent.h"
#include "TollCameraExtension.generated.h"

/**
 * @brief Type hooks for toll vehicle detection sensors (type keys "tolls:camera",
 *        "tolls:atobe:camera" and "tolls:atobe:lidar-tzc", struct FTollCameraData).
 *
 * Green "CAM" badge. Detections may spawn actors straight from WebSocket events, and
 * UTollCameraDriveComponent turns each detection into a short simulated drive.
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UTollCameraExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual FLinearColor GetBadgeColor() const override { return FLinearColor(0.557f, 0.882f, 0.533f); } // green
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("CAM"); }

    // Detections expire within seconds (attributes.expiry_ts): too short-lived for the
    // periodic tile-fetch/search index to reliably catch, so the live WS event is the only
    // realistic channel. See UEntityTypeExtension::AllowsOnDemandSpawnFromWS().
    virtual bool AllowsOnDemandSpawnFromWS() const override { return true; }

    // Each detection is a one-shot sighting of a (usually different) real vehicle, too sparse
    // to read as motion on its own: UTollCameraDriveComponent simulates continuous road motion
    // instead. See its class comment for why.
    virtual TSubclassOf<UEntityBehaviorComponent> GetBehaviorComponentClass() const override { return UTollCameraDriveComponent::StaticClass(); }
};
