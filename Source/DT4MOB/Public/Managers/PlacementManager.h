#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "PlacementManager.generated.h"

class AIgnitionPointGhostActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlacementModeChanged, bool, bActive);

/**
 * @brief LocalPlayer subsystem that owns the ignition-point placement mode.
 *
 * EnterPlacementMode() spawns a ghost preview actor that follows the terrain
 * cursor each tick. ConfirmPlacement() finalises the position, destroys the ghost,
 * and broadcasts OnPlacementConfirmed with the chosen world location.
 * CancelPlacement() tears down the ghost without placing anything.
 */
UCLASS()
class DT4MOB_API UPlacementManager : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    /** @brief Broadcast when placement mode is entered (true) or exited (false). */
    UPROPERTY(BlueprintAssignable)
    FOnPlacementModeChanged OnPlacementModeChanged;

    /** @brief Enter placement mode and spawn the ghost actor. */
    void EnterPlacementMode();

    /** @brief Confirm placement at WorldPos, destroy ghost, exit mode. */
    void ConfirmPlacement(const FVector& WorldPos);

    /** @brief Cancel placement, destroy ghost, exit mode. */
    void CancelPlacement();

    /** @brief Called every tick by UnifiedController to move the ghost to the current cursor hit. */
    void UpdateGhostPosition(const FVector& WorldPos, bool bHit);

    bool IsPlacing() const { return bPlacing; }

    void SetSelectedTypeKey(const FString& InTypeKey) { SelectedTypeKey = InTypeKey; }
    const FString& GetSelectedTypeKey() const { return SelectedTypeKey; }

private:
    bool bPlacing = false;
    FString SelectedTypeKey;

    UPROPERTY()
    AIgnitionPointGhostActor* GhostActor = nullptr;

    void ExitPlacementMode();
};
