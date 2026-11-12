// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UDT4MOBEntityFactory, which classifies Ditto things and spawns their actors.
 * @ingroup entities
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TempUIActor.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "DT4MOBEntityFactory.generated.h"

/**
 * @brief Display metadata for one registered entity type, used by the type dropdowns and the outline panel.
 * @ingroup entities
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FEntityTypeMeta
{
    GENERATED_USTRUCT_BODY()

    /**
     * @brief Registration key: the substring matched against thingIds (e.g. "tolls:camera").
     *
     * UEntityTypeDropdownWidget::OnTypeSelected broadcasts this value.
     */
    UPROPERTY(BlueprintReadOnly)
    FString TypeKey;

    /** @brief Human-readable label shown in the entity type dropdown. */
    UPROPERTY(BlueprintReadOnly)
    FString DisplayName;

    /** @brief If true, the dropdown warns that this type has no server-side handling. */
    UPROPERTY(BlueprintReadOnly)
    bool bNoServerHandling = false;

    /**
     * @brief Content path of the static mesh used as the default visual for this type.
     *
     * Empty keeps the actor's built-in Cube placeholder.
     */
    UPROPERTY(BlueprintReadOnly)
    FString DefaultMeshPath;
};

/**
 * @brief Game instance subsystem that classifies Ditto things by thingId and spawns their actors.
 *
 * Holds the entity type table filled by RegisterAllEntityTypes(): each thingId substring key
 * maps to a USTRUCT schema, display metadata and a UEntityTypeExtension. Lookups pick the
 * longest key contained in the thingId. SpawnTempUIActor() creates an ATempUIActor for a thing,
 * initialised with the matching struct and the raw JSON, and applies the type's default mesh
 * and any per-thing mesh overrides.
 *
 * The factory also tracks every actor it spawned, grouped by map tile, so the gamemode can
 * unload one tile or all of them. Actors that are on screen or have an open detail window
 * survive an unload as "orphans" until SweepOrphanedActors() finds them unprotected.
 *
 * Things whose thingId matches no key are written to disk by LogUnknownThing().
 *
 * @ingroup entities
 */
UCLASS()
class DT4MOB_API UDT4MOBEntityFactory : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** @brief Creates the shared default extension and fills the type table via RegisterAllEntityTypes(). */
    UDT4MOBEntityFactory();

    /**
     * @brief Spawns an ATempUIActor for a Ditto thing, or refreshes the one that already exists.
     *
     * In order:
     * - Returns nullptr if World or ThingData is invalid.
     * - Returns nullptr without spawning if `attributes.expiry_ts` is more than 30 s in the past
     *   (finished simulation runs leave expired things in Ditto).
     * - If UActorRegistryService already has a live actor for the thingId, re-runs
     *   ATempUIActor::Initialize() on it with the new JSON and returns it instead of spawning
     *   a duplicate.
     * - Returns nullptr and calls LogUnknownThing() if no registered key matches the thingId.
     *
     * Otherwise it spawns the actor at the origin, initialises it (which places it on the
     * globe), sets the type's default mesh if one is registered, and applies any per-thing mesh
     * overrides. For an override layer, a level AStaticMeshActor that already shows the same
     * mesh asset is reused if present; otherwise the mesh is loaded and added as a new layer.
     * The default Cube is hidden when overrides exist.
     *
     * @param World      World to spawn the actor in.
     * @param ThingData  Parsed Ditto thing JSON; must contain "thingId".
     * @return The new or existing actor, or nullptr if nothing was spawned.
     */
    ATempUIActor *SpawnTempUIActor(UWorld *World, TSharedPtr<FJsonObject> ThingData = nullptr);

    /**
     * @brief Resolves the USTRUCT schema for a Ditto thing from its thingId.
     *
     * Among all registered keys contained in the thingId, the longest one wins, so a more
     * specific key (".instrument.") beats a broader one ("geo-asset").
     *
     * @param ThingData  Parsed Ditto thing JSON containing a "thingId" field.
     * @return Matching struct type, or nullptr if ThingData is invalid or no key matches.
     */
    UScriptStruct *GetStructForThing(TSharedPtr<FJsonObject> ThingData);

    /**
     * @brief Checks whether any registered key is a substring of ThingId.
     *
     * Cheaper than GetStructForThing() when the caller only needs a yes/no answer and has
     * no JSON object.
     *
     * @param ThingId  Full Ditto thing identifier (e.g. "traci:vehicle-42").
     * @return True if the factory can spawn an actor for this thing.
     */
    bool CanHandleThingId(const FString &ThingId) const;

    /**
     * @brief Writes an unhandled thing's JSON to `<ProjectDir>/logs/<thingId>.json` for later analysis.
     *
     * Colons in the thingId become underscores in the file name, and an empty thingId is saved
     * as "UnknownThing". The directory is created if needed. An existing file is left alone.
     *
     * @param ThingData  Parsed Ditto thing JSON to save. Must be valid.
     */
    void LogUnknownThing(TSharedPtr<FJsonObject> ThingData);

    /**
     * @brief Lists the registered thingId substring keys (e.g. "traci", "tolls:toll").
     * @return All keys, in map order.
     */
    TArray<FString> GetRegisteredTypeKeys() const
    {
        TArray<FString> Keys;
        ThingStructMap.GetKeys(Keys);
        return Keys;
    }

    /**
     * @brief Finds the registered key for a thingId, using the same longest-match rule as GetStructForThing().
     * @param ThingId  Full Ditto thing identifier.
     * @return The longest registered key contained in ThingId, or an empty string if none matches.
     */
    FString GetTypeKeyForThingId(const FString& ThingId) const;

    /**
     * @brief Returns the behavior extension registered for a type key.
     * @param TypeKey  Registered key, as returned by GetTypeKeyForThingId().
     * @return The type's extension, or the shared default UEntityTypeExtension if the key is
     *         unknown or registered without one. Never null.
     */
    UEntityTypeExtension* GetExtensionForType(const FString& TypeKey) const;

    /**
     * @brief Resolves the type key for ThingId, then returns that type's extension.
     * @param ThingId  Full Ditto thing identifier.
     * @return Same as GetExtensionForType(). Never null.
     */
    UEntityTypeExtension* GetExtensionForThingId(const FString& ThingId) const
    {
        return GetExtensionForType(GetTypeKeyForThingId(ThingId));
    }

    /**
     * @brief Returns the display metadata registered for a type key.
     * @param Key  Registered type key.
     * @return The stored metadata, or, for an unknown key, one that uses Key as both TypeKey
     *         and DisplayName with bNoServerHandling false.
     */
    UFUNCTION(BlueprintPure)
    FEntityTypeMeta GetMetaForKey(const FString& Key) const
    {
        if (const FEntityTypeMeta* Meta = TypeMetaMap.Find(Key))
            return *Meta;
        return FEntityTypeMeta{Key, Key, false};
    }

    /**
     * @brief Returns metadata for every registered type, ordered for the dropdown.
     * @return All entries, with types that have server-side handling (bNoServerHandling false) first.
     */
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
     * @brief Destroys every tracked actor except protected ones, and forgets all tile assignments.
     *
     * Actors that are in camera view or have an open detail window (see ShouldProtectActor())
     * are kept alive, stay in SpawnedActors and are added to OrphanedActors. TileActorMap is
     * emptied. Call on the game thread.
     */
    void DestroyAllActors();

    /**
     * @brief Spawns (or reuses) an actor through SpawnTempUIActor() and records it under a tile key.
     *
     * If the actor was an orphan, it is removed from OrphanedActors because it has a tile again.
     *
     * @param World      World to spawn the actor in.
     * @param ThingData  Parsed Ditto thing JSON.
     * @param TileKey    Key of the map tile the thing was fetched for.
     * @return The actor, or nullptr if SpawnTempUIActor() returned nullptr.
     */
    ATempUIActor* SpawnTempUIActorForTile(UWorld* World, TSharedPtr<FJsonObject> ThingData, int64 TileKey);

    /**
     * @brief Destroys the actors recorded under a tile key and removes the key from TileActorMap.
     *
     * Protected actors (see ShouldProtectActor()) are not destroyed; they move to OrphanedActors.
     *
     * @param TileKey  Key the actors were registered under by SpawnTempUIActorForTile().
     */
    void DestroyActorsForTile(int64 TileKey);

    /**
     * @brief Checks whether any actor has been recorded under a tile key.
     *
     * The key is only added when SpawnTempUIActorForTile() returns an actor, so a tile whose
     * fetch produced no spawnable things still reports false.
     *
     * @param TileKey  Tile key to test.
     * @return True if TileActorMap has an entry for TileKey.
     */
    bool IsTileLoaded(int64 TileKey) const;

    /**
     * @brief Destroys orphaned actors that are no longer protected.
     *
     * An orphan is an actor kept alive past its tile's unload. Once it is out of camera view
     * and has no open window it is destroyed and removed from SpawnedActors. Stale weak
     * pointers are dropped. Called periodically from ADT4MOBGamemode::Tick().
     */
    void SweepOrphanedActors();

    /**
     * @brief Registers one Ditto thing type.
     *
     * Called once per type from RegisterAllEntityTypes() in EntityTypeRegistrations.cpp, which
     * holds the actual table so this header does not need every EntityStructs and
     * EntityDependencies include. When ExtensionClass is set, a new instance of it is created
     * and owned by the factory; otherwise the type uses the shared default extension.
     * Registering the same key twice replaces the earlier entry.
     *
     * @param Key                Substring matched against thingIds (e.g. "tolls:camera").
     * @param Struct             USTRUCT describing the thing's JSON schema.
     * @param DisplayName        Human-readable label shown in the entity type dropdown.
     * @param bNoServerHandling  Whether the dropdown should warn that this type has no server-side handling.
     * @param MeshPath           Optional content path of the default static mesh for this type.
     * @param ExtensionClass     Optional UEntityTypeExtension subclass with this type's custom behavior.
     */
    void RegisterType(const FString& Key, UScriptStruct* Struct, const FString& DisplayName, bool bNoServerHandling,
                       const FString& MeshPath = FString(), TSubclassOf<UEntityTypeExtension> ExtensionClass = nullptr);

    /**
     * @brief Registers mesh layers for one specific thing (not a whole type).
     *
     * SpawnTempUIActor() applies each path as a named mesh layer when it spawns this thing.
     * See ThingMeshOverrideMap.
     *
     * @param ThingId    Full Ditto thing identifier the override applies to.
     * @param MeshPaths  Static mesh content paths; each file's base name becomes the layer name.
     */
    void RegisterMeshOverride(const FString& ThingId, const TArray<FString>& MeshPaths);

private:
    /**
     * @brief Decides whether an actor must survive a tile unload.
     * @param Actor  Actor to test.
     * @return True if Actor is valid and is in camera view or has an open detail window.
     */
    static bool ShouldProtectActor(const ATempUIActor* Actor);

    /**
     * @brief Maps thingId substring keys to the USTRUCT that parses that thing type.
     *
     * A key matches any thingId that contains it (e.g. "tolls:camera"). When several keys
     * match, the longest one wins.
     */
    TMap<FString, UScriptStruct *> ThingStructMap;

    /** @brief Maps type keys to UI display metadata. */
    TMap<FString, FEntityTypeMeta> TypeMetaMap;

    /** @brief Maps type keys to their behavior extension instance (see EntityDependencies/). */
    UPROPERTY()
    TMap<FString, TObjectPtr<UEntityTypeExtension>> TypeExtensionMap;

    /** @brief Shared base UEntityTypeExtension used by types registered without an extension class. */
    UPROPERTY()
    TObjectPtr<UEntityTypeExtension> DefaultExtension;

    /** @brief Weak references to every actor this factory spawned and has not destroyed. Used for bulk cleanup. */
    TArray<TWeakObjectPtr<ATempUIActor>> SpawnedActors;

    /** @brief Actors grouped by the tile key they were fetched for, so one tile can be unloaded at a time. */
    TMap<int64, TArray<TWeakObjectPtr<ATempUIActor>>> TileActorMap;

    /**
     * @brief Protected actors (in camera view or with an open window) whose tile was unloaded.
     *
     * An orphan is removed from this list when SpawnTempUIActorForTile() returns it for a tile
     * again, or destroyed by SweepOrphanedActors() once it is no longer protected.
     */
    TArray<TWeakObjectPtr<ATempUIActor>> OrphanedActors;

    /** @brief Maps a full thingId to the static mesh paths applied to it as named mesh layers. */
    TMap<FString, TArray<FString>> ThingMeshOverrideMap;
};
