#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "Input/Reply.h"
#include "EntityWindowWidget.generated.h"

class ATempUIActor;
class UTextBlock;
class UButton;
class UBorder;
class USizeBox;
class UWidgetSwitcher;
class UJsonTabWidget;
class UInfoTabWidget;
class UAssocTabWidget;
class UModelsTabWidget;
class UInfoConfigPanelWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityWindowClosed, const FString&, ThingId);

UCLASS()
class DT4MOB_API UEntityWindowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;
    virtual void NativeDestruct() override;

    /** Binds this window to an actor. Call this after adding the widget to the viewport. */
    void OpenForActor(ATempUIActor* Actor);

    void BringToFront();

    /** Broadcast when the close button is pressed, before RemoveFromParent. */
    UPROPERTY(BlueprintAssignable)
    FOnEntityWindowClosed OnClosed;

protected:
    // ── Header ────────────────────────────────────────────────────────────────

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* EntityIdTitle;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* CloseButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TypeBadgeOutline;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TypeBadge;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TypeLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* StatusLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ThingIdSubLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* GrafanaButton;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EntityWindow")
    FString GrafanaUrlJsonPath = TEXT("attributes.grafana_url");

    // ── Tab bar ───────────────────────────────────────────────────────────────

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* TabInfoBtn;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* TabJsonBtn;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* TabAssocBtn;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* TabModelsBtn;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UWidgetSwitcher* TabSwitcher;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    USizeBox* WindowSizeBox;

    // ── Tab content ───────────────────────────────────────────────────────────

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UInfoTabWidget* InfoTabWidget;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UInfoConfigPanelWidget* ConfigPanel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UJsonTabWidget* JsonTabWidget;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UAssocTabWidget* AssocTabWidget;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UModelsTabWidget* ModelsTabWidget;

    // ── Blueprint hooks ───────────────────────────────────────────────────────

    UFUNCTION(BlueprintImplementableEvent, Category = "EntityWindow")
    void OnTabChanged(int32 NewTabIndex);

    UFUNCTION(BlueprintImplementableEvent, Category = "EntityWindow")
    void OnActorBound(bool bHasActor);

    /** Called when the config panel should slide in. Implement the animation in Blueprint. */
    UFUNCTION(BlueprintImplementableEvent, Category = "EntityWindow")
    void OnConfigPanelOpened();

    /** Called when the config panel should slide out. Implement the animation in Blueprint.
     *  Call CollapseConfigPanel() at the end of the animation to hide the panel. */
    UFUNCTION(BlueprintImplementableEvent, Category = "EntityWindow")
    void OnConfigPanelClosed();

    /** Collapses the config panel. Call this from Blueprint at the end of the slide-out animation. */
    UFUNCTION(BlueprintCallable, Category = "EntityWindow")
    void CollapseConfigPanel();

    // ── Window interaction ────────────────────────────────────────────────────

    /** Height of the draggable title bar area in pixels. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EntityWindow")
    float TitleBarHeight = 72.f;

    /** Size of the resize grip in the bottom-right corner in pixels. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EntityWindow")
    float ResizeGripSize = 20.f;

    /** Minimum window size when resizing. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EntityWindow")
    FVector2D MinWindowSize = FVector2D(300.f, 200.f);

    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
    enum class EDragMode : uint8 { None, Moving, Resizing };

    EDragMode CurrentDragMode = EDragMode::None;
    FVector2D DragStartMousePos;
    FVector2D DragStartWindowPos;
    FVector2D DragStartWindowSize;

    int32 ActiveTabIndex = 0;
    FString CachedGrafanaUrl;
    FString CachedThingId;

    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    class UCanvasPanelSlot* GetCanvasSlot() const;

    void BindToActor(ATempUIActor* Actor);
    void UnbindActor();
    void PopulateHeader();
    void SwitchToTab(int32 Index);

    UFUNCTION()
    void HandleCloseClicked();

    UFUNCTION()
    void HandleActorUnregistered(const FString& ThingId);

    UFUNCTION()
    void HandleGrafanaClicked();

    UFUNCTION()
    void HandleInfoConfigureRequested();

    UFUNCTION()
    void HandleConfigPanelClosed();

    UFUNCTION()
    void HandleTabInfoClicked();

    UFUNCTION()
    void HandleTabJsonClicked();

    UFUNCTION()
    void HandleTabAssocClicked();

    UFUNCTION()
    void HandleTabModelsClicked();
};
