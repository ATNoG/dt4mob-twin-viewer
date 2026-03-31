// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "DT4MOBPlayerCameraManager.generated.h"

/**
 * @brief Custom player camera manager for DT4MOB that constrains vertical look range.
 *
 * Sets ViewPitchMin and ViewPitchMax to ±89.9° so the camera cannot flip past vertical
 * in FreeFly mode, while still allowing nearly full vertical rotation.
 */
UCLASS()
class DT4MOB_API ADT4MOBPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	/** @brief Sets the pitch limits to prevent camera flipping (ViewPitchMin = -89.9, ViewPitchMax = 89.9). */
	ADT4MOBPlayerCameraManager();
};
