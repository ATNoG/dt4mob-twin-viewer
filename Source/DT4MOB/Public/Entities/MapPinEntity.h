// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapPinEntity.generated.h"

/**
 * @brief Simple actor that represents a map pin entity placed in the world.
 *
 * Stores geographic coordinates (latitude/longitude) and an entity identifier.
 * Intended as a lightweight marker; logic can be extended in derived classes or Blueprints.
 */
UCLASS()
class DT4MOB_API AMapPinEntity : public AActor
{
	GENERATED_BODY()

public:
	/** @brief Sets default values for this actor's properties. */
	AMapPinEntity();

protected:
	/** @brief Called when the game starts or when the actor is spawned. */
	virtual void BeginPlay() override;

public:
	/** @brief Called every frame. @param DeltaTime Time elapsed since the last frame. */
	virtual void Tick(float DeltaTime) override;

	/** @brief Unique identifier of the entity this pin represents. */
	FString entityID;

	/** @brief Geographic latitude of the pin in decimal degrees. */
	float latitude;

	/** @brief Geographic longitude of the pin in decimal degrees. */
	float longitude;
};
