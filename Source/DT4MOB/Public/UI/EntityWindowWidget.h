/**
 * @file
 * @brief Declares UEntityWindowWidget, the floating detail window opened for a selected entity.
 * @ingroup ui
 */

#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "Input/Reply.h"
#include "EntityWindowWidget.generated.h"

class ATempUIActor;
class UTextBlock;
class UButton;
class UBorder;
class UImage;
class USizeBox;
class UWidgetSwitcher;
class UJsonTabWidget;
class UInfoTabWidget;
class UAssocTabWidget;
class UModelsTabWidget;
class UInfoConfigPanelWidget;
class UWidgetAnimation;

/**
 * @brief Fired when an entity window closes, before it is removed from its parent.
 * @param ThingId  thingId of the entity the window was showing. Empty if no actor was bound.
 * @ingroup ui
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityWindowClosed, const FString&, ThingId);

/**
 * @brief Floating, draggable window that shows the details of one entity.
 *
 * URootHUDWidget creates one window per open entity inside its canvas panel and calls
 * OpenForActor(). The window has a header (thingId, type badge, status and an optional
 * Grafana link) and four tabs: Info (index 0), Raw JSON (1), Associated (2) and Models (3).
 * The Info tab can slide in a UInfoConfigPanelWidget that picks which fields it shows.
 *
 * Dragging the title bar moves the window and dragging the bottom-right grip resizes it;
 * both only work while the window sits in a UCanvasPanel slot. The window closes itself when
 * its actor is unregistered from UActorRegistryService.
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UEntityWindowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Binds the close, Grafana and tab buttons, hides the Grafana button and config panel,
     * subscribes to actor unregistration and hooks the end of the config panel slide-out animation.
     * @return False if the base class fails to initialize.
     */
    virtual bool Initialize() override;

    /** @brief Unsubscribes from the actor registry and unbinds the actor. */
    virtual void NativeDestruct() override;

    /**
     * @brief Binds this window to an actor and shows the Info tab.
     *
     * Call it after adding the window to its canvas panel and sizing the slot: it copies the
     * canvas slot size into WindowSizeBox before binding.
     *
     * @param Actor  Entity to show. Null or invalid clears the header and the tabs.
     */
    void OpenForActor(ATempUIActor* Actor);

    /**
     * @brief Draws this window above its siblings.
     *
     * Sets the canvas slot Z order to one more than the highest Z order among the parent
     * canvas panel's children. Does nothing if the window is not in a UCanvasPanel.
     */
    void BringToFront();

    /** @brief Broadcast with the entity's thingId when the window closes (close button or actor unregistered), before RemoveFromParent(). */
    UPROPERTY(BlueprintAssignable)
    FOnEntityWindowClosed OnClosed;

protected:
    // --- Header ---

    /** @brief Header title that shows the entity's thingId. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* EntityIdTitle;

    /** @brief Header button that closes the window. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* CloseButton;

    /** @brief Outline around the type badge. Optional; bound for the Blueprint layout, the C++ code does not change it. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TypeBadgeOutline;

    /** @brief Rounded badge behind TypeLabel, tinted with the entity type's badge color. Optional; set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TypeBadge;

    /** @brief Window frame, colored with the theme's WindowOutline. Must be named "WindowBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* WindowBorder;

    /** @brief Window body background, colored with the theme's BackgroundPrimary. Must be named "WidgetBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* WidgetBorder;

    /** @brief Header bar background. Must be named "HeaderBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* HeaderBorder;

    /** @brief Separator below the header. Must be named "Separator" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Separator;

    /** @brief Separator below the tab bar. Must be named "Separator2" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Separator2;

    /** @brief Short type label drawn in the badge color. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TypeLabel;

    /** @brief Status text. Shows "Active" while an actor is bound. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* StatusLabel;

    /** @brief Second header line, which also shows the thingId. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ThingIdSubLabel;

    /** @brief Opens the entity's Grafana URL in the browser. Collapsed when the entity has no URL. Optional; set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* GrafanaButton;

    /** @brief "Open in Grafana" link text. Must be named "GrafanaLink" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* GrafanaLink;

    /** @brief "Open in Grafana" arrow icon. Must be named "Arrow" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* Arrow;

    /** @brief Dot path into the entity's raw JSON that holds its Grafana dashboard URL. Read when the header is filled. Edited in the widget Blueprint defaults. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EntityWindow")
    FString GrafanaUrlJsonPath = TEXT("attributes.grafana_url");

    // --- Tab bar ---

    /** @brief Tab button that shows the Info tab (index 0). Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* TabInfoBtn;

    /** @brief Tab button that shows the Raw JSON tab (index 1). Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* TabJsonBtn;

    /** @brief Tab button that shows the Associated tab (index 2). Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* TabAssocBtn;

    /** @brief Tab button that shows the Models tab (index 3). Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* TabModelsBtn;

    /** @brief Info tab background: PanelBackground when active, TabInactiveBackground otherwise. Must be named "TabInfoBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TabInfoBorder;

    /** @brief Raw JSON tab background: PanelBackground when active, TabInactiveBackground otherwise. Must be named "TabJsonBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TabJsonBorder;

    /** @brief Associated tab background: PanelBackground when active, TabInactiveBackground otherwise. Must be named "TabAssocBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TabAssocBorder;

    /** @brief Models tab background: PanelBackground when active, TabInactiveBackground otherwise. Must be named "TabModelsBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TabModelsBorder;

    /** @brief Active-tab indicator line, visible only while the Info tab is active. Must be named "TabInfoIndicator" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* TabInfoIndicator;

    /** @brief Active-tab indicator line, visible only while the Raw JSON tab is active. Must be named "TabJsonIndicator" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* TabJsonIndicator;

    /** @brief Active-tab indicator line, visible only while the Associated tab is active. Must be named "TabAssocIndicator" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* TabAssocIndicator;

    /** @brief Active-tab indicator line, visible only while the Models tab is active. Must be named "TabModelsIndicator" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* TabModelsIndicator;

    /** @brief Info tab label text. Must be named "Info" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* Info;

    /** @brief Raw JSON tab label text. Must be named "RawJson" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* RawJson;

    /** @brief Associated tab label text. Must be named "Associated" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* Associated;

    /** @brief Models tab label text. Must be named "Models" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* Models;

    /** @brief Switcher that holds the tab pages in index order: Info, Raw JSON, Associated, Models. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UWidgetSwitcher* TabSwitcher;

    /** @brief Size box around the window content. Its size overrides follow the canvas slot on open and while resizing. Optional. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    USizeBox* WindowSizeBox;

    // --- Tab content ---

    /** @brief Info tab page: the configurable list of fields. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UInfoTabWidget* InfoTabWidget;

    /** @brief Slide-in panel that picks the Info tab fields. Collapsed until the Info tab asks for it. Optional. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UInfoConfigPanelWidget* ConfigPanel;

    /** @brief Raw JSON tab page. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UJsonTabWidget* JsonTabWidget;

    /** @brief Associated tab page: the entity's instrument things. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UAssocTabWidget* AssocTabWidget;

    /** @brief Models tab page: the actor's mesh layers. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UModelsTabWidget* ModelsTabWidget;

    // --- Blueprint hooks ---

    /**
     * @brief Blueprint hook called after the active tab changes.
     * @param NewTabIndex  0 Info, 1 Raw JSON, 2 Associated, 3 Models.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "EntityWindow")
    void OnTabChanged(int32 NewTabIndex);

    /**
     * @brief Blueprint hook called at the end of every bind.
     * @param bHasActor  True if the window now shows a valid actor.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "EntityWindow")
    void OnActorBound(bool bHasActor);

    /** @brief Slides the config panel in. Must be named "OpenConfigSlideAnimation" in the Blueprint's Animations panel. */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    UWidgetAnimation* OpenConfigSlideAnimation;

    /** @brief Slides the config panel out. Must be named "CloseConfigSlideAnimation" in the Blueprint's Animations panel. */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    UWidgetAnimation* CloseConfigSlideAnimation;

    /** @brief Collapses the config panel. Called when the slide-out animation finishes, or right away if there is no slide-out animation. */
    UFUNCTION(BlueprintCallable, Category = "EntityWindow")
    void CollapseConfigPanel();

    // --- Window interaction ---

    /** @brief Height in pixels of the draggable title bar area, measured from the top of the window. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EntityWindow")
    float TitleBarHeight = 72.f;

    /** @brief Side length in pixels of the square resize grip in the bottom-right corner. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EntityWindow")
    float ResizeGripSize = 20.f;

    /** @brief Minimum window size in pixels while resizing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EntityWindow")
    FVector2D MinWindowSize = FVector2D(300.f, 200.f);

    /**
     * @brief Brings the window to front on a left click and starts a drag.
     *
     * A click inside the resize grip starts a resize (and turns off slot auto-size); a click
     * in the title bar starts a move. Both capture the mouse.
     *
     * @param InGeometry    Geometry of this widget.
     * @param InMouseEvent  The mouse event.
     * @return Handled with mouse capture when a drag starts, otherwise the base class reply.
     */
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    /**
     * @brief Moves or resizes the window while a drag is active.
     *
     * Resizing keeps the size at or above MinWindowSize and updates WindowSizeBox. If the
     * canvas slot is gone, the drag is cancelled.
     *
     * @param InGeometry    Geometry of this widget.
     * @param InMouseEvent  The mouse event.
     * @return Handled during a drag, otherwise the base class reply.
     */
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    /**
     * @brief Ends a drag when the left button is released.
     * @param InGeometry    Geometry of this widget.
     * @param InMouseEvent  The mouse event.
     * @return Handled with mouse capture released if a drag ended, otherwise the base class reply.
     */
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    /**
     * @brief Colors the frame, header, separators and texts, stores the tab colors and refreshes the tab highlight.
     * @param Theme  Active theme. Ignored if null.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    /** @brief Background of the active tab (theme PanelBackground). */
    FLinearColor TabActiveColor = FLinearColor::Transparent;

    /** @brief Background of inactive tabs (theme TabInactiveBackground). */
    FLinearColor TabInactiveColor = FLinearColor::Transparent;

    /** @brief Label color of the active tab (theme TextPrimary). */
    FLinearColor TabTextActiveColor = FLinearColor::White;

    /** @brief Label color of inactive tabs (theme TextSecondary). */
    FLinearColor TabTextInactiveColor = FLinearColor::Gray;

    /** @brief Applies the active and inactive colors to the tab backgrounds and labels and shows only the active tab's indicator. */
    void RefreshTabHighlight();

    /**
     * @brief What the current left-button drag does.
     *
     * None: no drag in progress. Moving: the title bar drag moves the window.
     * Resizing: the grip drag resizes the window.
     */
    enum class EDragMode : uint8 { None, Moving, Resizing };

    /** @brief Drag in progress, if any. */
    EDragMode CurrentDragMode = EDragMode::None;

    /** @brief Mouse position in screen space when the drag started. */
    FVector2D DragStartMousePos;

    /** @brief Canvas slot position when a move started. */
    FVector2D DragStartWindowPos;

    /** @brief Widget size when a resize started. */
    FVector2D DragStartWindowSize;

    /** @brief Index of the visible tab: 0 Info, 1 Raw JSON, 2 Associated, 3 Models. */
    int32 ActiveTabIndex = 0;

    /** @brief Grafana URL read from GrafanaUrlJsonPath when the header was filled. Empty hides the Grafana button. */
    FString CachedGrafanaUrl;

    /** @brief thingId of the bound actor, kept so OnClosed and the unregister check work without the actor. */
    FString CachedThingId;

    /** @brief Entity shown in the window. */
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    /** @return This widget's slot as a UCanvasPanelSlot, or nullptr if the parent is not a canvas panel. */
    class UCanvasPanelSlot* GetCanvasSlot() const;

    /**
     * @brief Binds a new actor and passes it to every tab.
     *
     * Unbinds the previous actor, marks the new one as having an open window, fills the
     * header, switches to the Info tab and calls OnActorBound().
     *
     * @param Actor  Entity to show. May be null.
     * @note While an actor is marked as having an open window, UDT4MOBEntityFactory does not
     *       destroy it during a tile refresh.
     */
    void BindToActor(ATempUIActor* Actor);

    /** @brief Stops listening to the Info tab's configure request, clears the actor's open-window flag and forgets the actor. */
    void UnbindActor();

    /**
     * @brief Fills the header from the bound actor.
     *
     * Sets the title, thingId, type badge and status, and shows the Grafana button only if the
     * actor has a URL at GrafanaUrlJsonPath. Clears the texts when no actor is bound.
     */
    void PopulateHeader();

    /**
     * @brief Shows a tab, refreshes the tab highlight and calls OnTabChanged().
     * @param Index  Tab index, 0 to 3.
     */
    void SwitchToTab(int32 Index);

    /** @brief Plays OpenConfigSlideAnimation if it is bound. */
    void PlayConfigPanelOpenAnimation();

    /** @brief Plays CloseConfigSlideAnimation, or collapses the panel at once if that animation is not bound. */
    void PlayConfigPanelCloseAnimation();

    /** @brief Collapses the config panel when the slide-out animation ends. */
    UFUNCTION()
    void HandleConfigPanelCloseAnimationFinished();

    /** @brief Broadcasts OnClosed and removes the window from its parent. */
    UFUNCTION()
    void HandleCloseClicked();

    /**
     * @brief Closes the window when its own actor is unregistered.
     * @param ThingId  thingId of the actor that was unregistered.
     */
    UFUNCTION()
    void HandleActorUnregistered(const FString& ThingId);

    /** @brief Opens CachedGrafanaUrl in the system browser. */
    UFUNCTION()
    void HandleGrafanaClicked();

    /**
     * @brief Toggles the Info field config panel.
     *
     * If the panel is visible, slides it out. Otherwise sets it up for the bound actor's type,
     * makes it visible and slides it in.
     */
    UFUNCTION()
    void HandleInfoConfigureRequested();

    /** @brief Slides the config panel out after it saves or resets. */
    UFUNCTION()
    void HandleConfigPanelClosed();

    /** @brief Shows the Info tab. */
    UFUNCTION()
    void HandleTabInfoClicked();

    /** @brief Shows the Raw JSON tab. */
    UFUNCTION()
    void HandleTabJsonClicked();

    /** @brief Shows the Associated tab. */
    UFUNCTION()
    void HandleTabAssocClicked();

    /** @brief Shows the Models tab. */
    UFUNCTION()
    void HandleTabModelsClicked();
};
