#pragma once

/**
 * @file
 * @brief Declares UActorRegistryService, the ThingId -> ATempUIActor lookup table, and its delegates.
 * @ingroup services
 */

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ActorRegistryService.generated.h"

class ATempUIActor;

/**
 * @brief Fires when an actor is added to UActorRegistryService (not when the same actor is
 *        registered again under the same ThingId).
 * @param Actor  The newly registered actor.
 * @ingroup services
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorRegisteredDelegate, ATempUIActor*, Actor);

/**
 * @brief Fires when an entry is removed from UActorRegistryService.
 * @param ThingId  Id of the removed entry. The actor itself is not passed because it is usually
 *                 in EndPlay and about to be destroyed.
 * @ingroup services
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorUnregisteredDelegate, const FString&, ThingId);

/**
 * @brief GameInstance subsystem that keeps a global ThingId -> ATempUIActor map.
 *
 * ATempUIActor registers itself in BeginPlay once Initialize() has set its ThingId (or directly
 * in Initialize() when BeginPlay has already run) and unregisters in EndPlay. UI widgets, the
 * entity factory and the tile refresh look actors up here instead of iterating world actors.
 *
 * @note The map holds raw pointers that the garbage collector does not track. Entries stay
 *       valid only because every actor unregisters in EndPlay.
 *
 * @ingroup services
 */
UCLASS()
class DT4MOB_API UActorRegistryService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /**
     * @brief Finds the registry through the game instance of a world context object.
     * @param WorldContext  Any object that belongs to a world (actor, widget, component).
     * @return              The registry, or nullptr if WorldContext is null or has no world or
     *                      game instance.
     */
    static UActorRegistryService *Get(const UObject *WorldContext);

    /**
     * @brief Adds or replaces the entry for ThingId and broadcasts OnEntityRegistered.
     *
     * Registering the same actor again under the same ThingId does nothing and does not
     * broadcast, so listeners such as the outliner do not add duplicate rows.
     *
     * @param ThingId  Full Ditto thing id. Ignored if empty.
     * @param Actor    Actor to register. Ignored if null.
     */
    void RegisterActor(const FString &ThingId, ATempUIActor *Actor);

    /**
     * @brief Removes the entry for ThingId and broadcasts OnEntityUnregistered if one existed.
     * @param ThingId  Full Ditto thing id. Unknown ids are ignored.
     */
    void UnregisterActor(const FString &ThingId);

    /**
     * @brief Looks up an actor by exact ThingId (hash map lookup).
     * @param ThingId  Full Ditto thing id.
     * @return         The registered actor, or nullptr if none.
     */
    ATempUIActor *FindActor(const FString &ThingId) const;

    /**
     * @brief Returns every registered actor.
     * @return A copy of the registry values, in no particular order.
     */
    TArray<ATempUIActor *> GetAllActors() const;

    /**
     * @brief Returns the registered actors whose ThingId starts with Prefix.
     *
     * Used to find instrument children by passing parentThingId + ".instrument.". Linear in
     * the registry size, which is fine because it only runs on UI events.
     *
     * @param Prefix  Case-insensitive ThingId prefix (FString::StartsWith default).
     * @return        Matching actors, in no particular order; empty if none match.
     */
    TArray<ATempUIActor *> FindActorsWithPrefix(const FString &Prefix) const;

    /** @brief Broadcast when an actor joins the registry. Bound by the outliner and the associations tab. */
    UPROPERTY(BlueprintAssignable, Transient)
    FOnActorRegisteredDelegate OnEntityRegistered;

    /** @brief Broadcast when an entry leaves the registry. Carries the ThingId because the actor may be mid-destruction. */
    UPROPERTY(BlueprintAssignable, Transient)
    FOnActorUnregisteredDelegate OnEntityUnregistered;

private:
    /** @brief ThingId -> actor map. Raw pointers, not tracked by the garbage collector. */
    TMap<FString, ATempUIActor *> Registry;
};
