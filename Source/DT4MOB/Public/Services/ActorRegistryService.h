#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ActorRegistryService.generated.h"

class ATempUIActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorRegisteredDelegate, ATempUIActor*, Actor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorUnregisteredDelegate, const FString&, ThingId);

/**
 * @brief GameInstance subsystem that maintains a global ThingId → ATempUIActor registry.
 *
 * Actors register themselves on BeginPlay (after Initialize() sets their ThingId) and
 * unregister on EndPlay. Any system that needs to find actors by id — the UI, the factory,
 * tile refresh — queries this service instead of iterating world actors.
 */
UCLASS()
class DT4MOB_API UActorRegistryService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** @brief Convenience getter — returns nullptr if the subsystem is not available. */
    static UActorRegistryService *Get(const UObject *WorldContext);

    /** @brief Adds or overwrites the registry entry for the given ThingId. Broadcasts OnEntityRegistered. */
    void RegisterActor(const FString &ThingId, ATempUIActor *Actor);

    /** @brief Removes the registry entry for the given ThingId (no-op if not present). Broadcasts OnEntityUnregistered. */
    void UnregisterActor(const FString &ThingId);

    /** @brief O(1) lookup by exact ThingId. Returns nullptr if not found. */
    ATempUIActor *FindActor(const FString &ThingId) const;

    /** @brief Returns all currently registered actors. */
    TArray<ATempUIActor *> GetAllActors() const;

    /**
     * @brief Returns all registered actors whose ThingId starts with Prefix.
     *
     * Used to find instrument children: call with Prefix = parentThingId + ".instrument."
     * Complexity: O(n) over the registry — acceptable since this is called only on UI events.
     */
    TArray<ATempUIActor *> FindActorsWithPrefix(const FString &Prefix) const;

    /** Broadcast when an actor joins the registry. */
    UPROPERTY(BlueprintAssignable, Transient)
    FOnActorRegisteredDelegate OnEntityRegistered;

    /** Broadcast when an actor leaves the registry. ThingId is passed because the actor may be mid-destruction. */
    UPROPERTY(BlueprintAssignable, Transient)
    FOnActorUnregisteredDelegate OnEntityUnregistered;

private:
    TMap<FString, ATempUIActor *> Registry;
};
