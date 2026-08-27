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

    /** Overall window body background. Must be named "WindowBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* WindowBorder;

    /** Root widget background, behind WindowBorder. Must be named "WidgetBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* WidgetBorder;

    /** Header bar background. Must be named "HeaderBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* HeaderBorder;

    /** Separator below the header. Must be named "Separator" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Separator;

    /** Separator below the tab bar. Must be named "Separator2" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Separator2;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TypeLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* StatusLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ThingIdSubLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* GrafanaButton;

    /** "Open in Grafana" link text. Must be named "GrafanaLink" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* GrafanaLink;

    /** "Open in Grafana" arrow icon. Must be named "Arrow" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* Arrow;

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

    /** Active-tab underline. Must be named "TabInfoBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TabInfoBorder;

    /** Active-tab underline. Must be named "TabJsonBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TabJsonBorder;

    /** Active-tab underline. Must be named "TabAssocBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TabAssocBorder;

    /** Active-tab underline. Must be named "TabModelsBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TabModelsBorder;

    /** Active-tab indicator line. Must be named "TabInfoIndicator" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* TabInfoIndicator;

    /** Active-tab indicator line. Must be named "TabJsonIndicator" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* TabJsonIndicator;

    /** Active-tab indicator line. Must be named "TabAssocIndicator" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* TabAssocIndicator;

    /** Active-tab indicator line. Must be named "TabModelsIndicator" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* TabModelsIndicator;

    /** Tab label text. Must be named "Info" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* Info;

    /** Tab label text. Must be named "RawJson" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* RawJson;

    /** Tab label text. Must be named "Associated" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* Associated;

    /** Tab label text. Must be named "Models" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* Models;

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

    /** Slides the config panel in. Must be named "OpenConfigSlideAnimation" in the Blueprint's Animations panel. */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    UWidgetAnimation* OpenConfigSlideAnimation;

    /** Slides the config panel out. Must be named "CloseConfigSlideAnimation" in the Blueprint's Animations panel. */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    UWidgetAnimation* CloseConfigSlideAnimation;

    /** Collapses the config panel. Called automatically once the slide-out animation finishes. */
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

    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    FLinearColor TabActiveColor = FLinearColor::Transparent;
    FLinearColor TabInactiveColor = FLinearColor::Transparent;
    FLinearColor TabTextActiveColor = FLinearColor::White;
    FLinearColor TabTextInactiveColor = FLinearColor::Gray;

    void RefreshTabHighlight();

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
    void PlayConfigPanelOpenAnimation();
    void PlayConfigPanelCloseAnimation();

    UFUNCTION()
    void HandleConfigPanelCloseAnimationFinished();

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
