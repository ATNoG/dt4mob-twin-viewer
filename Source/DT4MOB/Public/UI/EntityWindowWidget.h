// EntityWindowWidget.h
#pragma once
#include "BaseWindowWidget.h"
#include "EntityWindowWidget.generated.h"

/**
 * @brief HUD widget that displays the name and live JSON data of a selected ATempUIActor.
 *
 * Subscribes to ATempUIActor::OnEntityDataChanged so the displayed JSON refreshes
 * automatically when the Daemon delivers live updates to the bound actor.
 * Bound widget components (NameText, DataText) must exist in the Blueprint layout.
 */
UCLASS()
class UEntityWindowWidget : public UBaseWindowWidget
{
    GENERATED_BODY()

public:
    /** @brief Text block displaying the actor's name. Must be bound in the Blueprint widget. */
    UPROPERTY(meta = (BindWidget))
    class UTextBlock *NameText;

    /** @brief Text block displaying the actor's raw JSON payload. Must be bound in the Blueprint widget. */
    UPROPERTY(meta = (BindWidget))
    class UTextBlock *DataText;

    /**
     * @brief Binds an actor to this window and populates NameText and DataText.
     *
     * Unbinds from the previous actor's OnEntityDataChanged delegate before binding
     * to the new one. If Actor is not an ATempUIActor, DataText is cleared.
     *
     * @param Actor The actor to display, or nullptr to clear the window.
     */
    virtual void OnBindData_Implementation(AActor *Actor) override;

    /**
     * @brief Closes the window and unbinds from the currently bound actor's delegate.
     */
    virtual void CloseWindow() override;

private:
    /** @brief Pointer to the actor whose data is currently being displayed. */
    UPROPERTY()
    class ATempUIActor *BoundActor = nullptr;

    /**
     * @brief Called when the bound ATempUIActor's OnEntityDataChanged fires.
     *
     * Refreshes DataText with the latest JSON string from the actor.
     */
    UFUNCTION()
    void HandleDataChanged();
};
