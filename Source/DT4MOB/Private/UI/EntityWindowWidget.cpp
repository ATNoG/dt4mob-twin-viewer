#include "UI/EntityWindowWidget.h"
#include "Entities/TempUIActor.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "UI/OutlineRowWidget.h"
#include "UI/InfoTabWidget.h"
#include "UI/JsonTabWidget.h"
#include "UI/AssocTabWidget.h"
#include "UI/ModelsTabWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/WidgetSwitcher.h"
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
    BindToActor(Actor);
}

// ── Binding ───────────────────────────────────────────────────────────────────

void UEntityWindowWidget::BindToActor(ATempUIActor* Actor)
{
    UnbindActor();
    BoundActor = Actor;
    CachedThingId = IsValid(Actor) ? Actor->GetThingId() : FString();

    if (InfoTabWidget)
        InfoTabWidget->SetBoundActor(BoundActor);

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
        FLinearColor BgColor = BadgeColor;
        BgColor.A = 0.18f;
        TypeBadge->SetBrushColor(BgColor);
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
