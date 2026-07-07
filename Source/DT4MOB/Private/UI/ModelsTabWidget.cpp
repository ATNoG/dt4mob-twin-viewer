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

    BoundActor = Actor;

    if (IsValid(BoundActor))
        BoundActor->OnMeshLayersChanged.AddDynamic(this, &UModelsTabWidget::HandleMeshLayersChanged);

    RebuildList();
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

    const TArray<FString> LayerNames = BoundActor->GetMeshLayerNames();

    if (SectionLabel)
        SectionLabel->SetText(FText::FromString(
            FString::Printf(TEXT("MESH LAYERS (%d)"), LayerNames.Num())));

    for (const FString& Name : LayerNames)
    {
        UModelsRowWidget* Row = CreateWidget<UModelsRowWidget>(GetOwningPlayer(), RowClass);
        if (!Row)
            continue;

        Row->SetEntry(BoundActor, Name);
        LayerList->AddChildToVerticalBox(Row);
    }
}

void UModelsTabWidget::HandleMeshLayersChanged()
{
    RebuildList();
}
