// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TempUIActor.h"
#include "DT4MOBEntityFactory.generated.h"

/**
 * @brief GameInstance subsystem responsible for spawning and classifying Ditto thing actors.
 *
 * Maintains a mapping from Ditto thing-type key substrings (e.g. "tolls:camera") to
 * UScriptStruct types. When a thing JSON arrives from the Ditto API, GetStructForThing()
 * matches the thingId against the map to select the correct struct, and SpawnTempUIActor()
 * instantiates an ATempUIActor initialised with that struct and the raw JSON payload.
 *
 * Things whose thingId does not match any registered key are silently logged to disk via
 * LogUnknownThing() for later inspection.
 */
UCLASS()
class DT4MOB_API UDT4MOBEntityFactory : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** @brief Populates ThingStructMap with all known Ditto thing-type to UScriptStruct mappings. */
    UDT4MOBEntityFactory();

    /**
     * @brief Spawns an ATempUIActor in World, initialised with the struct type inferred from ThingData.
     *
     * Returns nullptr and logs the thing to disk if the thingId is unknown or if any required
     * pointer is invalid.
     *
     * @param World      The world in which to spawn the actor.
     * @param ThingData  Parsed Ditto thing JSON object.
     * @return Pointer to the newly spawned ATempUIActor, or nullptr on failure.
     */
    ATempUIActor *SpawnTempUIActor(UWorld *World, TSharedPtr<FJsonObject> ThingData = nullptr);

    /**
     * @brief Resolves the UScriptStruct for the given Ditto thing JSON.
     *
     * Iterates ThingStructMap and returns the first entry whose key is contained in
     * the thing's thingId field.
     *
     * @param ThingData  Parsed Ditto thing JSON object containing a "thingId" field.
     * @return Matching UScriptStruct, or nullptr if no mapping exists.
     */
    UScriptStruct *GetStructForThing(TSharedPtr<FJsonObject> ThingData);

    /**
     * @brief Writes an unknown thing's JSON to a file under Project/logs/ for later analysis.
     *
     * The filename is derived from the thingId. Existing files are not overwritten.
     *
     * @param ThingData  Parsed Ditto thing JSON object to log.
     */
    void LogUnknownThing(TSharedPtr<FJsonObject> ThingData);

    /**
     * @brief Destroys all currently tracked actors spawned by this factory.
     *
     * Safe to call from the game thread at any time. Clears the internal registry
     * so subsequent SpawnTempUIActor calls start fresh.
     */
    void DestroyAllActors();

private:
    /**
     * @brief Maps Ditto thing-type substrings to their corresponding UScriptStruct types.
     *
     * Keys are substrings matched against the full thingId (e.g. "tolls:camera" matches
     * any thingId containing that string).
     */
    TMap<FString, UScriptStruct *> ThingStructMap;

    /** @brief Weak references to all actors spawned by this factory. Used for bulk cleanup. */
    TArray<TWeakObjectPtr<ATempUIActor>> SpawnedActors;
};
