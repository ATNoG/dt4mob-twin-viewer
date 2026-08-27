#include "UI/ModelsRowWidget.h"
#include "UI/OutlineRowWidget.h"
#include "Entities/TempUIActor.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"

bool UModelsRowWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (ToggleButton)
    {
        ToggleButton->OnClicked.AddDynamic(this, &UModelsRowWidget::HandleToggleClicked);
        ToggleButton->SetStyle(UOutlineRowWidget::MakePillButtonStyle());
    }

    if (TransparencyButton)
    {
        TransparencyButton->OnClicked.AddDynamic(this, &UModelsRowWidget::HandleTransparencyClicked);
        TransparencyButton->SetStyle(UOutlineRowWidget::MakePillButtonStyle());
    }

    return true;
}

void UModelsRowWidget::ApplyTheme_Implementation(UUIThemeData* Theme)
{
    if (!Theme) return;

    // RowBackgroundEven is an absolute dark gray tuned for the outline panel's own background —
    // against the Models tab's panel it's only ~2/255 different and reads as invisible. Use Hover
    // (a translucent white overlay) instead: it lightens relative to whatever's underneath, so it
    // stays visible regardless of the exact backdrop shade or theme.
    EvenRowColor = Theme->Hover;
    OddRowColor = FLinearColor::Transparent;
    RefreshRowBackground();
}

void UModelsRowWidget::SetEvenRow(bool bEven)
{
    bIsEvenRow = bEven;
    RefreshRowBackground();
}

void UModelsRowWidget::RefreshRowBackground()
{
    UOutlineRowWidget::SetBorderColorPreservingOutline(Border, bIsEvenRow ? EvenRowColor : OddRowColor);
}

void UModelsRowWidget::SetIndentLevel(int32 Level)
{
    if (!LayerNameLabel)
        return;

    LayerNameLabel->SetRenderTranslation(FVector2D(IndentPxPerLevel * Level, 0.f));

    FSlateFontInfo Font = LayerNameLabel->GetFont();
    Font.Size = FMath::Max(1, Font.Size - FontShrinkPerLevel * Level);
    LayerNameLabel->SetFont(Font);
}

void UModelsRowWidget::SetEntry(ATempUIActor* Actor, const FString& LayerName)
{
    BoundActor = Actor;
    CachedLayerName = LayerName;

    if (LayerNameLabel)
        LayerNameLabel->SetText(FText::FromString(LayerName));

    RefreshToggleLabel();
    RefreshTransparencyLabel();
}

void UModelsRowWidget::RefreshToggleLabel()
{
    if (!ToggleLabel || !IsValid(BoundActor))
        return;

    const bool bVisible = BoundActor->GetMeshLayerVisible(CachedLayerName);
    ToggleLabel->SetText(FText::FromString(bVisible ? TEXT("ON") : TEXT("OFF")));
}

void UModelsRowWidget::RefreshTransparencyLabel()
{
    if (!TransparencyLabel || !IsValid(BoundActor))
        return;

    const bool bTranslucent = BoundActor->GetMeshLayerTranslucent(CachedLayerName);
    TransparencyLabel->SetText(FText::FromString(bTranslucent ? TEXT("TRANSPARENT") : TEXT("OPAQUE")));
}

void UModelsRowWidget::HandleToggleClicked()
{
    if (!IsValid(BoundActor))
        return;

    const bool bNewVisible = !BoundActor->GetMeshLayerVisible(CachedLayerName);
    BoundActor->SetMeshLayerVisible(CachedLayerName, bNewVisible);
    RefreshToggleLabel();
    OnVisibilityToggled.Broadcast(CachedLayerName, bNewVisible);
}

void UModelsRowWidget::HandleTransparencyClicked()
{
    if (!IsValid(BoundActor))
        return;

    const bool bWantTranslucent = !BoundActor->GetMeshLayerTranslucent(CachedLayerName);
    BoundActor->SetMeshLayerTranslucent(CachedLayerName, bWantTranslucent);
    RefreshTransparencyLabel();

    // Reflect what actually happened — SetMeshLayerTranslucent is a no-op (state unchanged)
    // if no ghost material is configured in Project Settings.
    OnTransparencyToggled.Broadcast(CachedLayerName, BoundActor->GetMeshLayerTranslucent(CachedLayerName));
}
