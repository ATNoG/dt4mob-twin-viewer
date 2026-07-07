#include "UI/ToolbarWidget.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "Managers/PlacementManager.h"
#include "Gameplay/UnifiedPawn/UnifiedController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

bool UToolbarWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (APlayerController* PC = GetOwningPlayer())
    {
        PlacementManager = PC->GetLocalPlayer()->GetSubsystem<UPlacementManager>();
    }

    if (PlacementManager)
        PlacementManager->OnPlacementModeChanged.AddDynamic(this, &UToolbarWidget::HandlePlacementModeChanged);

    if (SwitchCameraButton && SwitchCameraButton->Button)
        SwitchCameraButton->Button->OnClicked.AddDynamic(this, &UToolbarWidget::HandleSwitchCameraClicked);

    if (PlaceEntityButton && PlaceEntityButton->Button)
        PlaceEntityButton->Button->OnClicked.AddDynamic(this, &UToolbarWidget::HandlePlaceEntityClicked);

    if (OutlineButton && OutlineButton->Button)
        OutlineButton->Button->OnClicked.AddDynamic(this, &UToolbarWidget::HandleOutlineClicked);

    if (UGameInstance* GI = GetGameInstance())
        EntityFactory = GI->GetSubsystem<UDT4MOBEntityFactory>();

    if (EntityTypeDropdown)
    {
        EntityTypeDropdown->OnTypeSelected.AddDynamic(this, &UToolbarWidget::HandleEntityTypeSelected);

        if (EntityFactory)
        {
            TArray<FString> SortedKeys;
            for (const FEntityTypeMeta& Meta : EntityFactory->GetRegisteredTypeEntries())
                SortedKeys.Add(Meta.TypeKey);
            EntityTypeDropdown->PopulateTypes(SortedKeys);
        }
    }

    // Disable spawn button until an entity type is selected
    if (PlaceEntityButton && PlaceEntityButton->Button)
        PlaceEntityButton->Button->SetIsEnabled(false);

    return true;
}

void UToolbarWidget::HandleSwitchCameraClicked()
{
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (AUnifiedController* Controller = Cast<AUnifiedController>(PC))
            Controller->ToggleCameraMode();
    }
}

void UToolbarWidget::HandlePlaceEntityClicked()
{
    if (!PlacementManager) return;

    if (PlacementManager->IsPlacing())
        PlacementManager->CancelPlacement();
    else
        PlacementManager->EnterPlacementMode();
}

void UToolbarWidget::HandleOutlineClicked()
{
    OnOutlineToggled.Broadcast();
}

void UToolbarWidget::HandleEntityTypeSelected(const FString& TypeKey)
{
    if (PlacementManager)
        PlacementManager->SetSelectedTypeKey(TypeKey);

    if (PlaceEntityButton && PlaceEntityButton->Button)
        PlaceEntityButton->Button->SetIsEnabled(!TypeKey.IsEmpty());

    if (EntityFactory)
    {
        const FEntityTypeMeta Meta = EntityFactory->GetMetaForKey(TypeKey);
        OnServerHandlingWarningChanged(!TypeKey.IsEmpty() && Meta.bNoServerHandling);
    }

    OnEntityTypeFilterChanged.Broadcast(TypeKey);
}

void UToolbarWidget::HandlePlacementModeChanged(bool bActive)
{
    if (PlaceEntityButton)
        PlaceEntityButton->SetActiveState(bActive);
}
