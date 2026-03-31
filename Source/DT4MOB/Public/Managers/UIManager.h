#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UIManager.generated.h"

/** @brief Broadcast when the entity detail window visibility changes. @param bVisible True if the window should be shown. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityWindowVisibilityChanged, bool, bVisible);

/**
 * @brief LocalPlayer subsystem that manages global UI state for the HUD.
 *
 * Currently tracks whether the entity-detail window should be visible.
 * Visibility is driven automatically by the SelectionManager: selecting an
 * actor shows the window and deselecting hides it.
 *
 * Consumers (e.g. URootHUDWidget) subscribe to OnEntityWindowVisibilityChanged
 * rather than polling IsEntityWindowVisible().
 */
UCLASS()
class DT4MOB_API UUIManager : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    /**
     * @brief Initialises the subsystem and binds to the SelectionManager's selection delegate.
     * @param Collection Subsystem collection provided by the engine.
     */
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;

    /** @brief Broadcast whenever the entity window visibility changes. */
    UPROPERTY(BlueprintAssignable)
    FOnEntityWindowVisibilityChanged OnEntityWindowVisibilityChanged;

    /**
     * @brief Explicitly sets the entity window visibility.
     *
     * No-ops if the new value equals the current state.
     * Broadcasts OnEntityWindowVisibilityChanged when the state changes.
     *
     * @param bVisible True to show the entity window, false to hide it.
     */
    void SetEntityWindowVisible(bool bVisible);

    /**
     * @brief Returns the current entity window visibility state.
     * @return True if the entity window is currently visible.
     */
    bool IsEntityWindowVisible() const { return bEntityWindowVisible; }

private:
    /** @brief Cached visibility state used to suppress redundant broadcasts. */
    bool bEntityWindowVisible = false;

    /**
     * @brief Bound to USelectionManager::OnSelectedActorChanged.
     *
     * Shows the window when an actor is selected and hides it when deselected.
     *
     * @param NewActor The newly selected actor, or nullptr.
     */
    UFUNCTION()
    void HandleSelectionChanged(AActor *NewActor);
};
