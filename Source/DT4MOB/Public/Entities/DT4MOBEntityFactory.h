// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TempUIActor.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "DT4MOBEntityFactory.generated.h"

/** @brief Metadata for a registered entity type, used by the UI dropdown. */
USTRUCT(BlueprintType)
struct DT4MOB_API FEntityTypeMeta
{
    GENERATED_USTRUCT_BODY()

    /** @brief Factory key used to match thingIds and passed to OnTypeSelected. */
    UPROPERTY(BlueprintReadOnly)
    FString TypeKey;

    /** @brief Human-readable label shown in the entity type dropdown. */
    UPROPERTY(BlueprintReadOnly)
    FString DisplayName;

    /** @brief If true, the dropdown shows a warning that this type has no server-side handling. */
    UPROPERTY(BlueprintReadOnly)
    bool bNoServerHandling = false;

    /** @brief Content path to the static mesh used as the default visual for this entity type.
     *  Empty = keep the actor's built-in Cube placeholder. */
    UPROPERTY(BlueprintReadOnly)
    FString DefaultMeshPath;
};

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
     * @brief Returns true if the factory has a registered struct for the given thingId string.
     *
     * Cheaper than GetStructForThing() when you only need to know whether a thing is handled —
     * avoids constructing a FJsonObject.
     *
     * @param ThingId  Full Ditto thing identifier (e.g. "traci:vehicle-42").
     */
    bool CanHandleThingId(const FString &ThingId) const;

    /**
     * @brief Writes an unknown thing's JSON to a file under Project/logs/ for later analysis.
     *
     * The filename is derived from the thingId. Existing files are not overwritten.
     *
     * @param ThingData  Parsed Ditto thing JSON object to log.
     */
    void LogUnknownThing(TSharedPtr<FJsonObject> ThingData);

    /** Returns the registered thingId substring keys (e.g. "traci", "tolls:toll"). */
    TArray<FString> GetRegisteredTypeKeys() const
    {
        TArray<FString> Keys;
        ThingStructMap.GetKeys(Keys);
        return Keys;
    }

    /** Returns the registered TypeKey whose substring matches ThingId (longest match wins). */
    FString GetTypeKeyForThingId(const FString& ThingId) const;

    /**
     * @brief Returns the registered extension for a type key, or a shared generic default
     *        (never null) if the type has no custom extension registered.
     */
    UEntityTypeExtension* GetExtensionForType(const FString& TypeKey) const;

    /** Convenience: resolves the type key from ThingId, then returns its extension. */
    UEntityTypeExtension* GetExtensionForThingId(const FString& ThingId) const
    {
        return GetExtensionForType(GetTypeKeyForThingId(ThingId));
    }

    /** Returns display metadata (DisplayName, bNoServerHandling) for a given type key. */
    UFUNCTION(BlueprintPure)
    FEntityTypeMeta GetMetaForKey(const FString& Key) const
    {
        if (const FEntityTypeMeta* Meta = TypeMetaMap.Find(Key))
            return *Meta;
        return FEntityTypeMeta{Key, Key, false};
    }

    /** Returns metadata for all registered types, ordered for the dropdown. */
    UFUNCTION(BlueprintPure)
    TArray<FEntityTypeMeta> GetRegisteredTypeEntries() const
    {
        TArray<FEntityTypeMeta> Entries;
        TypeMetaMap.GenerateValueArray(Entries);
        Entries.Sort([](const FEntityTypeMeta& A, const FEntityTypeMeta& B)
        {
            return A.bNoServerHandling < B.bNoServerHandling;
        });
        return Entries;
    }

    /**
     * @brief Destroys all currently tracked actors spawned by this factory.
     *
     * Safe to call from the game thread at any time. Clears the internal registry
     * so subsequent SpawnTempUIActor calls start fresh.
     */
    void DestroyAllActors();

    /** Same as SpawnTempUIActor but registers the actor under a tile quadkey for selective unloading. */
    ATempUIActor* SpawnTempUIActorForTile(UWorld* World, TSharedPtr<FJsonObject> ThingData, int64 TileKey);

    /** Destroys all actors that were spawned under the given tile key. */
    void DestroyActorsForTile(int64 TileKey);

    /** Returns true if the given tile key has at least one actor registered (i.e. was already fetched). */
    bool IsTileLoaded(int64 TileKey) const;

    /** @brief Destroys any orphaned actor (kept alive past its tile's unload) that is no longer
     *  in camera view and has no open window. Called periodically from ADT4MOBGamemode::Tick(). */
    void SweepOrphanedActors();

    /**
     * @brief Registers a single Ditto thing-type. Called once per type from RegisterAllEntityTypes()
     *        (see Entities/EntityTypeRegistrations.cpp) — the actual registration table lives there
     *        instead of in this class so this file doesn't need to include every EntityStruct/
     *        EntityDependencies header just to add a type.
     *
     * @param Key                Thing-type substring matched against thingId (e.g. "tolls:camera").
     * @param Struct              UScriptStruct describing the thing's JSON schema.
     * @param DisplayName         Human-readable label shown in the entity type dropdown.
     * @param bNoServerHandling   Whether the dropdown should warn this type has no server-side handling.
     * @param MeshPath            Optional content path to the default static mesh for this type.
     * @param ExtensionClass      Optional UEntityTypeExtension subclass for this type's custom behavior.
     */
    void RegisterType(const FString& Key, UScriptStruct* Struct, const FString& DisplayName, bool bNoServerHandling,
                       const FString& MeshPath = FString(), TSubclassOf<UEntityTypeExtension> ExtensionClass = nullptr);

    /** @brief Registers one or more content paths applied as named mesh layers for a single,
     *  specific thingId (not a type) — see ThingMeshOverrideMap. */
    void RegisterMeshOverride(const FString& ThingId, const TArray<FString>& MeshPaths);

private:
    /** @brief Returns true if Actor must not be destroyed during a tile refresh
     *  (currently in camera view, or has an open detail window). */
    static bool ShouldProtectActor(const ATempUIActor* Actor);
    /**
     * @brief Maps Ditto thing-type substrings to their corresponding UScriptStruct types.
     *
     * Keys are substrings matched against the full thingId (e.g. "tolls:camera" matches
     * any thingId containing that string).
     */
    TMap<FString, UScriptStruct *> ThingStructMap;

    /** @brief Maps type keys to UI display metadata (DisplayName, bNoServerHandling). */
    TMap<FString, FEntityTypeMeta> TypeMetaMap;

    /** @brief Maps type keys to their registered behavior extension (see EntityDependencies/). */
    UPROPERTY()
    TMap<FString, TObjectPtr<UEntityTypeExtension>> TypeExtensionMap;

    /** @brief Shared instance returned by GetExtensionForType() for types with no custom extension. */
    UPROPERTY()
    TObjectPtr<UEntityTypeExtension> DefaultExtension;

    /** @brief Weak references to all actors spawned by this factory. Used for bulk cleanup. */
    TArray<TWeakObjectPtr<ATempUIActor>> SpawnedActors;

    /** @brief Tracks which actors belong to which tile quadkey for selective unloading. */
    TMap<int64, TArray<TWeakObjectPtr<ATempUIActor>>> TileActorMap;

    /** @brief Protected actors (in camera view or with an open window) whose tile was unloaded
     *  out from under them. Re-homed into TileActorMap if their tile is fetched again, or
     *  destroyed by SweepOrphanedActors() once they're no longer protected. */
    TArray<TWeakObjectPtr<ATempUIActor>> OrphanedActors;

    /** @brief Maps a full thingId to one or more content paths applied as named mesh layers. */
    TMap<FString, TArray<FString>> ThingMeshOverrideMap;
};
