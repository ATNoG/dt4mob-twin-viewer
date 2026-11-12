// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares ADT4MOBPlayerCameraManager, which limits camera pitch to just short of vertical.
 * @ingroup gameplay
 */

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "DT4MOBPlayerCameraManager.generated.h"

/**
 * @brief Player camera manager that clamps view pitch to -89.9 to 89.9 degrees.
 *
 * Stopping just short of straight up or down keeps the FreeFly camera from flipping over.
 * APlayerCameraManager's own constructor uses the same values, so this class only states
 * them explicitly.
 *
 * @ingroup gameplay
 */
UCLASS()
class DT4MOB_API ADT4MOBPlayerCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

public:
	/** @brief Sets ViewPitchMin to -89.9 and ViewPitchMax to 89.9 degrees, and logs a warning-level trace line. */
	ADT4MOBPlayerCameraManager();
};
