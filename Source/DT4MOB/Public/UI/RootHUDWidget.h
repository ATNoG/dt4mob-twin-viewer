#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Managers/SelectionManager.h"
#include "Components/CanvasPanel.h"
#include "RootHUDWidget.generated.h"

class UEntityWindowWidget;
class UToolbarWidget;
class ATempUIActor;

/**
 * @brief Top-level HUD widget that owns all other HUD panels.
 *
 * On Initialize() it wires up:
 *  - USelectionManager to show/hide the EntityWindow when actors are selected.
 *  - UToolbarWidget::OnOutlineToggled to toggle the OutlinePanel.
 *
 * All child widget references must be bound in the Blueprint layout.
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
    // Entity Windows
    // -----------------------

    /** @brief Canvas panel used as the container for dynamically spawned instrument windows. Must be bound in Blueprint. */
    UPROPERTY(meta = (BindWidget))
    UCanvasPanel *WindowContainer;

    /** @brief Widget class used to spawn additional instrument windows. Set in Blueprint defaults. */
    UPROPERTY(EditDefaultsOnly, Category = "UI")
    TSubclassOf<UEntityWindowWidget> EntityWindowClass;

    /**
     * @brief Opens (or brings to front) a floating window for the given actor.
     *
     * If a window for Actor is already open it is brought to the top of the Z-order.
     * Otherwise a new UEntityWindowWidget is created inside WindowContainer.
     * Callable from Blueprint — instrument buttons use this to open child windows.
     */
    UFUNCTION(BlueprintCallable)
    void OpenWindowForActor(ATempUIActor *Actor);

    /**
     * @brief Closes and removes the floating window for the given actor, if one is open.
     */
    UFUNCTION(BlueprintCallable)
    void CloseWindowForActor(ATempUIActor *Actor);

    /** @brief Toolbar with tool buttons (e.g. place ignition point). Must be bound in Blueprint. */
    UPROPERTY(meta = (BindWidget))
    UToolbarWidget *Toolbar;

    /** @brief Cached pointer to the LocalPlayer SelectionManager subsystem. */
    UPROPERTY()
    USelectionManager *SelectionSubsystem;

    /** @brief All currently open floating instrument windows, keyed by their bound actor. */
    UPROPERTY()
    TMap<ATempUIActor *, UEntityWindowWidget *> OpenWindows;

    // -----------------------
    // Event handlers
    // -----------------------

    UFUNCTION()
    void HandleSelectionChanged(AActor *SelectedActor);

    /** Called when UToolbarWidget::OnOutlineToggled fires. Toggles OutlinePanel visibility. */
    UFUNCTION()
    void HandleOutlineToggled();
};
