// Fill out your copyright notice in the Description page of Project Settings.

/** @file DT4MOBPlayerCameraManager.cpp
 *  @brief Implementation of ADT4MOBPlayerCameraManager. All logic documentation is in the header.
 */
#include "Gameplay/DT4MOBPlayerCameraManager.h"

ADT4MOBPlayerCameraManager::ADT4MOBPlayerCameraManager()
{
    // log
    UE_LOG(LogTemp, Warning, TEXT("DT4MOBPlayerCameraManager Constructor"));

    ViewPitchMin = -89.9f;
    ViewPitchMax = 89.9f;
}