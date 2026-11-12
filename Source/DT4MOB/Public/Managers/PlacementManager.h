/**
 * @file
 * @brief Declares UPlacementManager, the local player subsystem behind the entity placement tool.
 * @ingroup managers
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "PlacementManager.generated.h"

class AIgnitionPointGhostActor;

/**
 * @brief Fires when placement mode starts or ends.
 * @param bActive True when placement mode was entered, false when it was left (confirmed or cancelled).
 * @ingroup managers
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlacementModeChanged, bool, bActive);

/**
 * @brief Local player subsystem that tracks placement mode and its ghost preview actor.
 *
 * EnterPlacementMode() spawns an AIgnitionPointGhostActor (the same preview for every type)
 * that AUnifiedController moves to the terrain under the cursor each tick through
 * UpdateGhostPosition(). ConfirmPlacement() and CancelPlacement() both destroy the ghost and
 * leave placement mode. This class does not create entities: on a confirming click
 * AUnifiedController builds the thing JSON for GetSelectedTypeKey(), spawns the actor and PUTs
 * it to Ditto.
 *
 * @ingroup managers
 */
UCLASS()
class DT4MOB_API UPlacementManager : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    /** @brief Broadcast with true when placement mode is entered and false when it is exited. UToolbarWidget listens to it. */
    UPROPERTY(BlueprintAssignable)
    FOnPlacementModeChanged OnPlacementModeChanged;

    /**
     * @brief Enters placement mode and spawns the ghost actor, initially hidden at the world origin.
     *
     * Does nothing if placement mode is already active. Broadcasts OnPlacementModeChanged(true).
     */
    void EnterPlacementMode();

    /**
     * @brief Ends placement mode after a confirming click: destroys the ghost and broadcasts OnPlacementModeChanged(false).
     *
     * Does nothing if placement mode is not active. The position is not stored or used here;
     * AUnifiedController creates the entity itself after calling this.
     *
     * @param WorldPos  Chosen world position (currently unused by this function).
     */
    void ConfirmPlacement(const FVector& WorldPos);

    /** @brief Leaves placement mode without placing anything: destroys the ghost and broadcasts OnPlacementModeChanged(false). Does nothing if not placing. */
    void CancelPlacement();

    /**
     * @brief Moves and shows or hides the ghost. Called every tick by AUnifiedController while placing.
     *
     * Does nothing if no ghost exists.
     *
     * @param WorldPos  Cursor hit position in Unreal world space; only used when bHit is true.
     * @param bHit      True if the cursor trace hit terrain. The ghost is hidden when false.
     */
    void UpdateGhostPosition(const FVector& WorldPos, bool bHit);

    /**
     * @brief Returns whether placement mode is active.
     * @return True between EnterPlacementMode() and the next confirm or cancel.
     */
    bool IsPlacing() const { return bPlacing; }

    /**
     * @brief Sets the entity type key to place (e.g. "fire:"). Called by the toolbar when the user picks a type.
     * @param InTypeKey  Type key registered with UDT4MOBEntityFactory.
     */
    void SetSelectedTypeKey(const FString& InTypeKey) { SelectedTypeKey = InTypeKey; }

    /**
     * @brief Returns the entity type key to place.
     * @return The selected type key, or an empty string if none was set (AUnifiedController then uses "fire:").
     */
    const FString& GetSelectedTypeKey() const { return SelectedTypeKey; }

private:
    /** @brief True while placement mode is active. */
    bool bPlacing = false;

    /** @brief Type key chosen through SetSelectedTypeKey(); empty until one is set. */
    FString SelectedTypeKey;

    /** @brief Ghost preview spawned by EnterPlacementMode(), or nullptr outside placement mode. */
    UPROPERTY()
    AIgnitionPointGhostActor* GhostActor = nullptr;

    /** @brief Clears bPlacing, destroys the ghost and broadcasts OnPlacementModeChanged(false). */
    void ExitPlacementMode();
};
