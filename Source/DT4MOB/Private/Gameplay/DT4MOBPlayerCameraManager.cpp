// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Implements ADT4MOBPlayerCameraManager (camera pitch limits).
 * @ingroup gameplay
 */
#include "Gameplay/DT4MOBPlayerCameraManager.h"

ADT4MOBPlayerCameraManager::ADT4MOBPlayerCameraManager()
{
    // Construction trace, logged at Warning level.
    UE_LOG(LogTemp, Warning, TEXT("DT4MOBPlayerCameraManager Constructor"));

    ViewPitchMin = -89.9f;
    ViewPitchMax = 89.9f;
}