#include "UI/EntityWindowWidget.h"
#include "Entities/TempUIActor.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "UI/OutlineRowWidget.h"
#include "UI/InfoTabWidget.h"
#include "UI/InfoConfigPanelWidget.h"
#include "UI/InfoFieldRegistry.h"
#include "Engine/LocalPlayer.h"
#include "UI/JsonTabWidget.h"
#include "UI/AssocTabWidget.h"
#include "UI/ModelsTabWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/WidgetSwitcher.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Engine/GameInstance.h"

bool UEntityWindowWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (CloseButton)
        CloseButton->OnClicked.AddDynamic(this, &UEntityWindowWidget::HandleCloseClicked);

    if (GrafanaButton)
    {
        GrafanaButton->OnClicked.AddDynamic(this, &UEntityWindowWidget::HandleGrafanaClicked);
        GrafanaButton->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (ConfigPanel)
    {
        ConfigPanel->SetVisibility(ESlateVisibility::Collapsed);
        ConfigPanel->OnClosed.AddDynamic(this, &UEntityWindowWidget::HandleConfigPanelClosed);
    }

    if (TabInfoBtn)
        TabInfoBtn->OnClicked.AddDynamic(this, &UEntityWindowWidget::HandleTabInfoClicked);

    if (TabJsonBtn)
        TabJsonBtn->OnClicked.AddDynamic(this, &UEntityWindowWidget::HandleTabJsonClicked);

    if (TabAssocBtn)
        TabAssocBtn->OnClicked.AddDynamic(this, &UEntityWindowWidget::HandleTabAssocClicked);

    if (TabModelsBtn)
        TabModelsBtn->OnClicked.AddDynamic(this, &UEntityWindowWidget::HandleTabModelsClicked);

    return true;
}

void UEntityWindowWidget::NativeDestruct()
{
    UnbindActor();
    Super::NativeDestruct();
}

// ── Public API ────────────────────────────────────────────────────────────────

void UEntityWindowWidget::OpenForActor(ATempUIActor* Actor)
{
    // Sync SizeBox to whatever the canvas slot was initialised to.
    if (WindowSizeBox)
    {
        if (UCanvasPanelSlot* CanSlot = GetCanvasSlot())
        {
            const FVector2D S = CanSlot->GetSize();
            WindowSizeBox->SetWidthOverride(S.X);
            WindowSizeBox->SetHeightOverride(S.Y);
        }
    }
    BindToActor(Actor);
}

// ── Binding ───────────────────────────────────────────────────────────────────

void UEntityWindowWidget::BindToActor(ATempUIActor* Actor)
{
    UnbindActor();
    BoundActor = Actor;
    CachedThingId = IsValid(Actor) ? Actor->GetThingId() : FString();

    if (InfoTabWidget)
    {
        InfoTabWidget->OnConfigureRequested.AddDynamic(this, &UEntityWindowWidget::HandleInfoConfigureRequested);
        InfoTabWidget->SetBoundActor(BoundActor);
    }

    if (JsonTabWidget)
        JsonTabWidget->SetBoundActor(BoundActor);

    if (AssocTabWidget)
        AssocTabWidget->SetBoundActor(BoundActor);

    if (ModelsTabWidget)
        ModelsTabWidget->SetBoundActor(BoundActor);

    PopulateHeader();
    SwitchToTab(0);
    OnActorBound(IsValid(BoundActor));
}

void UEntityWindowWidget::UnbindActor()
{
    if (InfoTabWidget)
        InfoTabWidget->OnConfigureRequested.RemoveDynamic(this, &UEntityWindowWidget::HandleInfoConfigureRequested);
    BoundActor = nullptr;
}

// ── Header ────────────────────────────────────────────────────────────────────

void UEntityWindowWidget::PopulateHeader()
{
    if (!IsValid(BoundActor))
    {
        if (EntityIdTitle)   EntityIdTitle->SetText(FText::GetEmpty());
        if (TypeLabel)       TypeLabel->SetText(FText::GetEmpty());
        if (StatusLabel)     StatusLabel->SetText(FText::GetEmpty());
        if (ThingIdSubLabel) ThingIdSubLabel->SetText(FText::GetEmpty());
        return;
    }

    const FString& ThingId = BoundActor->GetThingId();

    if (EntityIdTitle)
        EntityIdTitle->SetText(FText::FromString(ThingId));

    if (ThingIdSubLabel)
        ThingIdSubLabel->SetText(FText::FromString(ThingId));

    FString TypeKey;
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UDT4MOBEntityFactory* Factory = GI->GetSubsystem<UDT4MOBEntityFactory>())
            TypeKey = Factory->GetTypeKeyForThingId(ThingId);
    }

    const FString BadgeLabel = UOutlineRowWidget::GetBadgeLabel(TypeKey);
    const FLinearColor BadgeColor = UOutlineRowWidget::GetBadgeColor(TypeKey);

    if (TypeLabel)
    {
        TypeLabel->SetText(FText::FromString(BadgeLabel));
        TypeLabel->SetColorAndOpacity(FSlateColor(BadgeColor));
    }

    if (TypeBadge)
    {
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
        Brush.TintColor = FSlateColor(BadgeColor.CopyWithNewOpacity(0.15f));
        Brush.OutlineSettings.Width = 1.f;
        Brush.OutlineSettings.Color = FSlateColor(BadgeColor.CopyWithNewOpacity(0.4f));
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        Brush.OutlineSettings.CornerRadii = FVector4(0.f, 0.f, 0.f, 0.f);
        TypeBadge->SetBrush(Brush);
    }

    if (StatusLabel)
        StatusLabel->SetText(FText::FromString(TEXT("Active")));

    CachedGrafanaUrl = BoundActor->GetRawJsonField(GrafanaUrlJsonPath);
    if (GrafanaButton)
        GrafanaButton->SetVisibility(CachedGrafanaUrl.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

// ── Tabs ──────────────────────────────────────────────────────────────────────

void UEntityWindowWidget::SwitchToTab(int32 Index)
{
    ActiveTabIndex = Index;

    if (TabSwitcher)
        TabSwitcher->SetActiveWidgetIndex(ActiveTabIndex);

    OnTabChanged(ActiveTabIndex);
}

void UEntityWindowWidget::HandleTabInfoClicked()   { SwitchToTab(0); }
void UEntityWindowWidget::HandleTabJsonClicked()   { SwitchToTab(1); }
void UEntityWindowWidget::HandleTabAssocClicked()  { SwitchToTab(2); }
void UEntityWindowWidget::HandleTabModelsClicked() { SwitchToTab(3); }

// ── Buttons ───────────────────────────────────────────────────────────────────

// ── Drag & Resize ─────────────────────────────────────────────────────────────

UCanvasPanelSlot* UEntityWindowWidget::GetCanvasSlot() const
{
    return Cast<UCanvasPanelSlot>(Slot);
}

void UEntityWindowWidget::BringToFront()
{
    UCanvasPanelSlot* CanSlot = GetCanvasSlot();
    UCanvasPanel* Parent = Cast<UCanvasPanel>(GetParent());
    if (!CanSlot || !Parent)
        return;

    int32 MaxZ = 0;
    for (int32 i = 0; i < Parent->GetChildrenCount(); i++)
    {
        if (UWidget* Child = Parent->GetChildAt(i))
            if (UCanvasPanelSlot* ChildSlot = Cast<UCanvasPanelSlot>(Child->Slot))
                MaxZ = FMath::Max(MaxZ, ChildSlot->GetZOrder());
    }
    CanSlot->SetZOrder(MaxZ + 1);
}

FReply UEntityWindowWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

    BringToFront();

    UCanvasPanelSlot* CanSlot = GetCanvasSlot();
    TSharedPtr<SWidget> ThisSlate = GetCachedWidget();
    if (!CanSlot || !ThisSlate.IsValid())
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

    const FVector2D LocalPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    const FVector2D WidgetSize = InGeometry.GetLocalSize();

    DragStartMousePos = InMouseEvent.GetScreenSpacePosition();

    // Resize: bottom-right corner grip
    if (LocalPos.X >= WidgetSize.X - ResizeGripSize && LocalPos.Y >= WidgetSize.Y - ResizeGripSize)
    {
        CurrentDragMode = EDragMode::Resizing;
        // Freeze auto-size so we can drive size manually
        DragStartWindowSize = WidgetSize;
        CanSlot->SetAutoSize(false);
        CanSlot->SetSize(DragStartWindowSize);
        return FReply::Handled().CaptureMouse(ThisSlate.ToSharedRef());
    }

    // Move: title bar area
    if (LocalPos.Y <= TitleBarHeight)
    {
        CurrentDragMode = EDragMode::Moving;
        DragStartWindowPos = CanSlot->GetPosition();
        return FReply::Handled().CaptureMouse(ThisSlate.ToSharedRef());
    }

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UEntityWindowWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (CurrentDragMode == EDragMode::None)
        return Super::NativeOnMouseMove(InGeometry, InMouseEvent);

    UCanvasPanelSlot* CanSlot = GetCanvasSlot();
    if (!CanSlot)
    {
        CurrentDragMode = EDragMode::None;
        return FReply::Handled();
    }

    const FVector2D Delta = InMouseEvent.GetScreenSpacePosition() - DragStartMousePos;

    if (CurrentDragMode == EDragMode::Moving)
    {
        CanSlot->SetPosition(DragStartWindowPos + Delta);
    }
    else
    {
        const FVector2D NewSize = (DragStartWindowSize + Delta).ComponentMax(MinWindowSize);
        CanSlot->SetSize(NewSize);
        if (WindowSizeBox)
        {
            WindowSizeBox->SetWidthOverride(NewSize.X);
            WindowSizeBox->SetHeightOverride(NewSize.Y);
        }
    }

    return FReply::Handled();
}

FReply UEntityWindowWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton || CurrentDragMode == EDragMode::None)
        return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);

    CurrentDragMode = EDragMode::None;
    return FReply::Handled().ReleaseMouseCapture();
}

// ── Buttons ───────────────────────────────────────────────────────────────────

void UEntityWindowWidget::HandleCloseClicked()
{
    OnClosed.Broadcast(CachedThingId);
    RemoveFromParent();
}

void UEntityWindowWidget::HandleGrafanaClicked()
{
    if (CachedGrafanaUrl.IsEmpty())
        return;

    FPlatformProcess::LaunchURL(*CachedGrafanaUrl, nullptr, nullptr);
}

void UEntityWindowWidget::HandleInfoConfigureRequested()
{
    if (!ConfigPanel || !IsValid(BoundActor))
        return;

    if (ConfigPanel->GetVisibility() != ESlateVisibility::Collapsed)
    {
        HandleConfigPanelClosed();
        return;
    }

    FString TypeKey;
    if (UGameInstance* GI = GetGameInstance())
        if (UDT4MOBEntityFactory* Factory = GI->GetSubsystem<UDT4MOBEntityFactory>())
            TypeKey = Factory->GetTypeKeyForThingId(BoundActor->GetThingId());

    if (ULocalPlayer* LP = GetOwningLocalPlayer())
        if (UInfoFieldRegistry* Reg = LP->GetSubsystem<UInfoFieldRegistry>())
            ConfigPanel->Setup(BoundActor, TypeKey, Reg);

    ConfigPanel->SetVisibility(ESlateVisibility::Visible);
    OnConfigPanelOpened();
}

void UEntityWindowWidget::HandleConfigPanelClosed()
{
    OnConfigPanelClosed();
    // Blueprint calls CollapseConfigPanel() at the end of its slide-out animation.
    // If OnConfigPanelClosed is not overridden in Blueprint, collapse immediately.
}

void UEntityWindowWidget::CollapseConfigPanel()
{
    if (ConfigPanel)
        ConfigPanel->SetVisibility(ESlateVisibility::Collapsed);
}
