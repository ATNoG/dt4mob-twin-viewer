/**
 * @file
 * @brief Declares URootHUDWidget, the top-level HUD that hosts the toolbar, outline panel, entity windows and Options window.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ToolbarWidget.h"
#include "UI/OutlinePanelWidget.h"
#include "UI/EntityWindowWidget.h"
#include "UI/OptionsWindowWidget.h"
#include "RootHUDWidget.generated.h"

class UUIManager;
class UCanvasPanel;

/**
 * @brief Top-level HUD widget of the main scene.
 *
 * Wires the toolbar to the outline panel and the Options window, and opens one floating
 * UEntityWindowWidget per entity when the entity is selected in the world
 * (USelectionManager::OnSelectedActorChanged) or clicked in the outline panel. Selecting an
 * entity whose window is already open brings that window to the front instead.
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API URootHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Caches UUIManager, subscribes to USelectionManager::OnSelectedActorChanged and binds
     *        the toolbar and outline panel events.
     * @return False if the base UUserWidget::Initialize() fails, true otherwise.
     */
    virtual bool Initialize() override;

protected:
    /** @brief Toolbar whose Outline, entity type and Options events this widget handles. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UToolbarWidget* Toolbar;

    /** @brief Slide-out entity list, toggled from the toolbar. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UOutlinePanelWidget* OutlinePanel;

    /** @brief Full-screen canvas that entity windows are added to. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UCanvasPanel* EntityWindowContainer;

    /** @brief Widget class created for each entity window. Set in the WBP_RootHud defaults; if unset, no windows open. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EntityWindow")
    TSubclassOf<UEntityWindowWidget> EntityWindowClass;

    /**
     * @brief Full-screen modal opened from the toolbar's Options button. Optional; must be named
     *        "OptionsWindow" in the widget Blueprint.
     *
     * It is placed last in the layout so it paints above Toolbar, OutlinePanel and EntityWindowContainer.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UOptionsWindowWidget* OptionsWindow;

    /** @brief UUIManager of the owning local player, cached in Initialize(). Not used by this widget at the moment. */
    UPROPERTY()
    UUIManager* UIManager = nullptr;

private:
    /** @brief ThingId -> open window. Weak pointers, so windows that were closed and destroyed expire on their own. */
    TMap<FString, TWeakObjectPtr<UEntityWindowWidget>> OpenWindows;

    /**
     * @brief Brings the actor's window to the front if it is open; otherwise creates a new one.
     *
     * A new window is added to EntityWindowContainer at 520 x 600, offset 30 px further right and
     * down for each window already open, and bound to Actor with UEntityWindowWidget::OpenForActor().
     *
     * @param Actor  Entity to show; callers pass only valid actors.
     */
    void SpawnOrFocusWindow(ATempUIActor* Actor);

    /** @brief Toggles OutlinePanel. */
    UFUNCTION()
    void HandleOutlineToggled();

    /** @brief Opens OptionsWindow. */
    UFUNCTION()
    void HandleOptionsRequested();

    /**
     * @brief Does nothing yet; the type filter is not applied anywhere.
     * @param TypeKey  Selected entity type key, empty for no filter.
     */
    UFUNCTION()
    void HandleEntityTypeFilterChanged(const FString& TypeKey);

    /**
     * @brief Opens or focuses the window of a newly selected entity. Deselection and actors that are not ATempUIActor are ignored.
     * @param Actor  Newly selected actor, or null when the selection was cleared.
     */
    UFUNCTION()
    void HandleEntitySelected(AActor* Actor);

    /**
     * @brief Removes a closed window from OpenWindows.
     * @param ThingId  Ditto thingId of the window that closed.
     */
    UFUNCTION()
    void HandleEntityWindowClosed(const FString& ThingId);

    /**
     * @brief Opens or focuses the window of an entity clicked in the outline panel.
     * @param Actor  Actor of the clicked row; ignored if not valid.
     */
    UFUNCTION()
    void HandleOutlineEntityOpenRequested(ATempUIActor* Actor);
};
