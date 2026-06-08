#include "UI/ModelsRowWidget.h"
#include "Entities/TempUIActor.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

bool UModelsRowWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (ToggleButton)
        ToggleButton->OnClicked.AddDynamic(this, &UModelsRowWidget::HandleToggleClicked);

    return true;
}

void UModelsRowWidget::SetEntry(ATempUIActor* Actor, const FString& LayerName)
{
    BoundActor = Actor;
    CachedLayerName = LayerName;

    if (LayerNameLabel)
        LayerNameLabel->SetText(FText::FromString(LayerName));

    RefreshToggleLabel();
}

void UModelsRowWidget::RefreshToggleLabel()
{
    if (!ToggleLabel || !IsValid(BoundActor))
        return;

    const bool bVisible = BoundActor->GetMeshLayerVisible(CachedLayerName);
    ToggleLabel->SetText(FText::FromString(bVisible ? TEXT("ON") : TEXT("OFF")));
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
