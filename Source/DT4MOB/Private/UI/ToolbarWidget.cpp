#include "UI/ToolbarWidget.h"
#include "Managers/PlacementManager.h"
#include "Kismet/GameplayStatics.h"

bool UToolbarWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PlacementManager = PC->GetLocalPlayer()->GetSubsystem<UPlacementManager>();
    }

    if (PlacementManager)
    {
        PlacementManager->OnPlacementModeChanged.AddDynamic(this, &UToolbarWidget::HandlePlacementModeChanged);
    }

    if (PlaceIgnitionPointButton)
    {
        PlaceIgnitionPointButton->OnClicked.AddDynamic(this, &UToolbarWidget::HandlePlaceIgnitionPointClicked);
    }

    return true;
}

void UToolbarWidget::HandlePlaceIgnitionPointClicked()
{
    if (!PlacementManager)
        return;

    if (PlacementManager->IsPlacing())
        PlacementManager->CancelPlacement();
    else
        PlacementManager->EnterPlacementMode();
}

void UToolbarWidget::HandlePlacementModeChanged(bool bActive)
{
    if (!PlaceIgnitionPointButton)
        return;

    // Tint the button to show active state
    FLinearColor Tint = bActive ? FLinearColor(1.f, 0.4f, 0.1f, 1.f) : FLinearColor::White;
    PlaceIgnitionPointButton->SetColorAndOpacity(Tint);
}
