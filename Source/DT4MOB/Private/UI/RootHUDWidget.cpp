/** @file RootHUDWidget.cpp
 *  @brief Implementation of URootHUDWidget. All logic documentation is in the header.
 */
#include "UI/RootHUDWidget.h"
#include "UI/EntityWindowWidget.h"
#include "Gameplay/UnifiedPawn/UnifiedController.h"
#include "Kismet/GameplayStatics.h"

bool URootHUDWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    // -----------------------
    // Grab the Interaction Subsystem
    // -----------------------
    if (APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        SelectionSubsystem = PC->GetLocalPlayer()->GetSubsystem<USelectionManager>();
    }

    // -----------------------
    // Bind selection event
    // -----------------------
    if (SelectionSubsystem)
    {
        SelectionSubsystem->OnSelectedActorChanged.AddDynamic(this, &URootHUDWidget::HandleSelectionChanged);
    }

    // Optional: start with entity window hidden
    if (EntityWindow)
    {
        EntityWindow->SetVisibility(ESlateVisibility::Collapsed);
        EntityWindow->InitCanvasSlot();
    }
    // -----------------------
    // Bind button events (if buttons exist in the widget)

    if (ToggleCameraModeButton)
    {
        ToggleCameraModeButton->OnClicked.AddDynamic(this, &URootHUDWidget::HandleToggleCameraModeClicked);
    }

    return true;
}

// -----------------------
// Event handler for selection changes
// -----------------------
void URootHUDWidget::HandleSelectionChanged(AActor *SelectedActor)
{
    if (!EntityWindow)
        return;

    if (SelectedActor)
    {
        EntityWindow->OpenWindow();
        EntityWindow->OnBindData(SelectedActor);
    }
    else
    {
        EntityWindow->CloseWindow();
    }
}

void URootHUDWidget::HandleToggleCameraModeClicked()
{
    if (APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (AUnifiedController *UnifiedController = Cast<AUnifiedController>(PC))
        {
            UnifiedController->ToggleCameraMode();
        }
    }
}
