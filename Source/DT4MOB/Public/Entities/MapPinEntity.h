// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares AMapPinEntity, a minimal map pin actor with an id and a latitude/longitude.
 * @ingroup entities
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapPinEntity.generated.h"

/**
 * @brief Minimal actor that holds an entity id and a latitude/longitude pair for a map pin.
 *
 * It has no placement, rendering or update logic of its own, and no C++ code in the module
 * spawns it or reads its fields. Ditto things are represented by ATempUIActor instead.
 * Derived classes or Blueprints can build on it.
 *
 * @ingroup entities
 */
UCLASS()
class DT4MOB_API AMapPinEntity : public AActor
{
	GENERATED_BODY()

public:
	/** @brief Enables ticking; there is no other setup. */
	AMapPinEntity();

protected:
	/** @brief Calls the base implementation only. */
	virtual void BeginPlay() override;

public:
	/**
	 * @brief Calls the base implementation only.
	 * @param DeltaTime  Seconds since the last frame.
	 */
	virtual void Tick(float DeltaTime) override;

	/** @brief Id of the entity this pin stands for. Not a UPROPERTY, so Blueprints cannot see it. */
	FString entityID;

	/** @brief Latitude of the pin in decimal degrees (WGS-84). */
	float latitude;

	/** @brief Longitude of the pin in decimal degrees (WGS-84). */
	float longitude;
};
