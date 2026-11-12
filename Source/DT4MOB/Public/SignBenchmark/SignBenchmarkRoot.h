// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Actor that owns every component spawned by one sign benchmark run.
 * @ingroup signbenchmark
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SignBenchmarkRoot.generated.h"

/**
 * @brief Owns every component spawned by one benchmark run (the static mesh components or ISMs of a single strategy).
 *
 * ASignBenchmarkGameMode destroys it on SignBenchClear and spawns a new one at the world origin on
 * each SignBenchSpawn, instead of cleaning components up one by one, so nothing from a previous
 * run can leak into the next measurement.
 * @ingroup signbenchmark
 */
UCLASS()
class DT4MOB_API ASignBenchmarkRoot : public AActor
{
    GENERATED_BODY()

public:
    /** @brief Disables ticking and creates a Static-mobility scene component as the root that spawned components attach to. */
    ASignBenchmarkRoot();
};
