#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Managers/SelectionManager.h"
#include "Components/Button.h"
#include "RootHUDWidget.generated.h"

class UPlayerInteractionSubsystem;
class UEntityWindowWidget;
class AActor;

/**
 * @brief Top-level HUD widget that owns all other HUD panels.
 *
 * On Initialize() it wires up:
 *  - USelectionManager to show/hide the EntityWindow when actors are selected.
 *  - The ToggleCameraModeButton to switch the pawn between RTS and FreeFly modes.
 *
 * All child widget and button references must be bound in the Blueprint layout.
 */
UCLASS()
class URootHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Performs widget initialisation, binds selection and button delegates.
     * @return True if the parent initialisation succeeded.
     */
    virtual bool Initialize() override;

protected:
    // -----------------------
    // Entity Window (conditional)
    // -----------------------

    /** @brief Panel shown when an entity actor is selected. Must be bound in the Blueprint layout. */
    UPROPERTY(meta = (BindWidget))
    UEntityWindowWidget *EntityWindow;

    // -----------------------
    // Buttons
    // -----------------------

    /** @brief Button that toggles the pawn camera mode between RTS and FreeFly. Must be bound in the Blueprint layout. */
    UPROPERTY(meta = (BindWidget))
    UButton *ToggleCameraModeButton;

    // -----------------------
    // Subsystem reference
    // -----------------------

    /** @brief Cached pointer to the LocalPlayer SelectionManager subsystem. */
    UPROPERTY()
    USelectionManager *SelectionSubsystem;

    // -----------------------
    // Event handlers
    // -----------------------

    /**
     * @brief Called when the selected actor changes.
     *
     * Opens EntityWindow and binds data when an actor is selected; closes it when deselected.
     *
     * @param SelectedActor The newly selected actor, or nullptr.
     */
    UFUNCTION()
    void HandleSelectionChanged(AActor *SelectedActor);

    /**
     * @brief Called when the ToggleCameraModeButton is clicked.
     *
     * Delegates to AUnifiedController::ToggleCameraMode() on the player controller.
     */
    UFUNCTION()
    void HandleToggleCameraModeClicked();
};
