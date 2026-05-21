#include "UI/ToolbarWidget.h"
#include "Managers/PlacementManager.h"
#include "Gameplay/UnifiedPawn/UnifiedController.h"
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

    if (SwitchCameraButton)
    {
        SwitchCameraButton->OnClicked.AddDynamic(this, &UToolbarWidget::HandleSwitchCameraClicked);
    }

    if (PlaceEntityButton)
    {
        PlaceEntityButton->OnClicked.AddDynamic(this, &UToolbarWidget::HandlePlaceEntityClicked);
    }

    if (OutlineButton)
    {
        OutlineButton->OnClicked.AddDynamic(this, &UToolbarWidget::HandleOutlineClicked);
    }

    if (EntityTypeComboBox)
    {
        EntityTypeComboBox->AddOption(TEXT("All"));
        EntityTypeComboBox->AddOption(TEXT("car"));
        EntityTypeComboBox->AddOption(TEXT("toll"));
        EntityTypeComboBox->AddOption(TEXT("road"));
        EntityTypeComboBox->AddOption(TEXT("sensor"));
        EntityTypeComboBox->SetSelectedOption(TEXT("All"));
        EntityTypeComboBox->OnSelectionChanged.AddDynamic(this, &UToolbarWidget::HandleEntityTypeSelectionChanged);
    }

    return true;
}

void UToolbarWidget::HandleSwitchCameraClicked()
{
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (AUnifiedController* UnifiedController = Cast<AUnifiedController>(PC))
        {
            UnifiedController->ToggleCameraMode();
        }
    }
}

void UToolbarWidget::HandlePlaceEntityClicked()
{
    if (!PlacementManager)
        return;

    if (PlacementManager->IsPlacing())
        PlacementManager->CancelPlacement();
    else
        PlacementManager->EnterPlacementMode();
}

void UToolbarWidget::HandleOutlineClicked()
{
    OnOutlineToggled.Broadcast();
}

void UToolbarWidget::HandleEntityTypeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    OnEntityTypeFilterChanged.Broadcast(SelectedItem);
}

void UToolbarWidget::HandlePlacementModeChanged(bool bActive)
{
    if (!PlaceEntityButton)
        return;

    // Tint the button to reflect active placement state.
    const FLinearColor Tint = bActive ? FLinearColor(1.f, 0.4f, 0.1f, 1.f) : FLinearColor::White;
    PlaceEntityButton->SetColorAndOpacity(Tint);
}
