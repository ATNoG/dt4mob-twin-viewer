#include "UI/ModelsGroupRowWidget.h"
#include "UI/OutlineRowWidget.h"
#include "Entities/TempUIActor.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"

bool UModelsGroupRowWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (ToggleButton)
    {
        ToggleButton->OnClicked.AddDynamic(this, &UModelsGroupRowWidget::HandleToggleClicked);
        ToggleButton->SetStyle(UOutlineRowWidget::MakePillButtonStyle());
    }

    if (TransparencyButton)
    {
        TransparencyButton->OnClicked.AddDynamic(this, &UModelsGroupRowWidget::HandleTransparencyClicked);
        TransparencyButton->SetStyle(UOutlineRowWidget::MakePillButtonStyle());
    }

    if (ExpandButton)
        ExpandButton->OnClicked.AddDynamic(this, &UModelsGroupRowWidget::HandleExpandClicked);

    if (Arrow)
    {
        // Force a centered pivot regardless of what the Blueprint's Render Transform is set to —
        // an off-center pivot (e.g. default top-left) rotates the icon around a corner instead of
        // its center, swinging part of it outside the slot and clipping/ghosting against neighbors.
        Arrow->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
    }

    SetExpanded(bIsExpanded);

    return true;
}

void UModelsGroupRowWidget::ApplyTheme_Implementation(UUIThemeData* Theme)
{
    if (!Theme) return;

    UOutlineRowWidget::SetBorderColorPreservingOutline(Border, Theme->BackgroundSecondary);
}

void UModelsGroupRowWidget::SetEntry(ATempUIActor* Actor, const FString& InGroupName, TSubclassOf<UModelsRowWidget> ChildRowClass)
{
    BoundActor = Actor;
    GroupName = InGroupName;

    if (LayerNameLabel)
        LayerNameLabel->SetText(FText::FromString(GroupName));

    RefreshToggleLabel();
    RefreshTransparencyLabel();
    PopulateChildren(ChildRowClass);
}

void UModelsGroupRowWidget::PopulateChildren(TSubclassOf<UModelsRowWidget> ChildRowClass)
{
    if (!ChildList || !ChildRowClass || !IsValid(BoundActor))
        return;

    ChildList->ClearChildren();

    int32 Index = 0;
    for (const FString& MemberName : BoundActor->GetMeshLayerGroupMembers(GroupName))
    {
        UModelsRowWidget* Row = CreateWidget<UModelsRowWidget>(GetOwningPlayer(), ChildRowClass);
        if (!Row)
            continue;

        Row->SetEntry(BoundActor, MemberName);
        Row->SetIndentLevel(1);
        Row->SetEvenRow(Index % 2 == 0);
        ChildList->AddChildToVerticalBox(Row);
        ++Index;
    }
}

void UModelsGroupRowWidget::RefreshToggleLabel()
{
    if (!ToggleLabel || !IsValid(BoundActor))
        return;

    const bool bVisible = BoundActor->IsLayerGroupVisible(GroupName);
    ToggleLabel->SetText(FText::FromString(bVisible ? TEXT("ON") : TEXT("OFF")));
}

void UModelsGroupRowWidget::RefreshTransparencyLabel()
{
    if (!TransparencyLabel || !IsValid(BoundActor))
        return;

    const bool bTranslucent = BoundActor->IsLayerGroupTranslucent(GroupName);
    TransparencyLabel->SetText(FText::FromString(bTranslucent ? TEXT("TRANSPARENT") : TEXT("OPAQUE")));
}

void UModelsGroupRowWidget::HandleToggleClicked()
{
    if (!IsValid(BoundActor))
        return;

    const bool bNewVisible = !BoundActor->IsLayerGroupVisible(GroupName);
    BoundActor->SetLayerGroupVisible(GroupName, bNewVisible);
    RefreshToggleLabel();
}

void UModelsGroupRowWidget::HandleTransparencyClicked()
{
    if (!IsValid(BoundActor))
        return;

    const bool bWantTranslucent = !BoundActor->IsLayerGroupTranslucent(GroupName);
    BoundActor->SetLayerGroupTranslucent(GroupName, bWantTranslucent);
    RefreshTransparencyLabel();
}

void UModelsGroupRowWidget::HandleExpandClicked()
{
    SetExpanded(!bIsExpanded);
    OnExpandedChanged.Broadcast(GroupName, bIsExpanded);
}

void UModelsGroupRowWidget::SetExpanded(bool bExpanded)
{
    bIsExpanded = bExpanded;

    if (ChildList)
        ChildList->SetVisibility(bIsExpanded ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    if (Arrow)
        Arrow->SetRenderTransformAngle(bIsExpanded ? 0.f : -90.f);
}
