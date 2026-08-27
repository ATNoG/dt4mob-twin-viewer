#include "UI/ModelsTabWidget.h"
#include "Entities/TempUIActor.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"

void UModelsTabWidget::NativeDestruct()
{
    SetBoundActor(nullptr);
    Super::NativeDestruct();
}

void UModelsTabWidget::SetBoundActor(ATempUIActor* Actor)
{
    if (IsValid(BoundActor))
        BoundActor->OnMeshLayersChanged.RemoveDynamic(this, &UModelsTabWidget::HandleMeshLayersChanged);

    if (Actor != BoundActor)
        ExpandedGroups.Empty();

    BoundActor = Actor;

    if (IsValid(BoundActor))
        BoundActor->OnMeshLayersChanged.AddDynamic(this, &UModelsTabWidget::HandleMeshLayersChanged);

    RebuildList();
}

void UModelsTabWidget::ApplyTheme_Implementation(UUIThemeData* Theme)
{
    if (!Theme) return;

    if (SectionLabel)
        SectionLabel->SetColorAndOpacity(FSlateColor(Theme->TextPrimary));
}

void UModelsTabWidget::RebuildList()
{
    if (!LayerList)
        return;

    LayerList->ClearChildren();

    if (!IsValid(BoundActor) || !RowClass)
    {
        if (SectionLabel)
            SectionLabel->SetText(FText::FromString(TEXT("MESH LAYERS")));
        return;
    }

    if (SectionLabel)
        SectionLabel->SetText(FText::FromString(
            FString::Printf(TEXT("MESH LAYERS (%d)"), BoundActor->GetMeshLayerNames().Num())));

    if (GroupRowClass)
    {
        for (const FString& Group : BoundActor->GetMeshLayerGroupNames())
        {
            // A "group" with a single member is named identically to its one layer
            // (see AddOrReplaceMeshLayerGroup) — no separate header row needed for it.
            if (BoundActor->GetMeshLayerGroupMembers(Group).Num() <= 1)
                continue;

            UModelsGroupRowWidget* GroupRow = CreateWidget<UModelsGroupRowWidget>(GetOwningPlayer(), GroupRowClass);
            if (!GroupRow)
                continue;

            GroupRow->SetEntry(BoundActor, Group, RowClass);
            GroupRow->SetExpanded(ExpandedGroups.Contains(Group));
            GroupRow->OnExpandedChanged.AddDynamic(this, &UModelsTabWidget::HandleGroupExpandedChanged);
            LayerList->AddChildToVerticalBox(GroupRow);
        }
    }

    int32 Index = 0;
    for (const FString& Name : BoundActor->GetUngroupedMeshLayerNames())
    {
        UModelsRowWidget* Row = CreateWidget<UModelsRowWidget>(GetOwningPlayer(), RowClass);
        if (!Row)
            continue;

        Row->SetEntry(BoundActor, Name);
        Row->SetEvenRow(Index % 2 == 0);
        LayerList->AddChildToVerticalBox(Row);
        ++Index;
    }
}

void UModelsTabWidget::HandleMeshLayersChanged()
{
    RebuildList();
}

void UModelsTabWidget::HandleGroupExpandedChanged(const FString& GroupName, bool bExpanded)
{
    if (bExpanded)
        ExpandedGroups.Add(GroupName);
    else
        ExpandedGroups.Remove(GroupName);
}
