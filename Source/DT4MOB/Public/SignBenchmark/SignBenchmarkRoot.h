// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SignBenchmarkRoot.generated.h"

/**
 * @brief Owns every component spawned by one benchmark run (a single strategy's
 * meshes/ISMs). Destroyed and respawned fresh on each SignBenchClear/SignBenchSpawn
 * rather than incrementally cleaned up, so a stale instance can never leak into the
 * next measurement.
 */
UCLASS()
class DT4MOB_API ASignBenchmarkRoot : public AActor
{
    GENERATED_BODY()

public:
    ASignBenchmarkRoot();
};
