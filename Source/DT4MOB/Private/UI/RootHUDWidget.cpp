#include "UI/RootHUDWidget.h"
#include "Managers/UIManager.h"
#include "Managers/SelectionManager.h"
#include "Entities/TempUIActor.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

bool URootHUDWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (APlayerController* PC = GetOwningPlayer())
    {
        UIManager = PC->GetLocalPlayer()->GetSubsystem<UUIManager>();

        if (USelectionManager* SM = PC->GetLocalPlayer()->GetSubsystem<USelectionManager>())
            SM->OnSelectedActorChanged.AddDynamic(this, &URootHUDWidget::HandleEntitySelected);
    }

    if (Toolbar)
    {
        Toolbar->OnOutlineToggled.AddDynamic(this, &URootHUDWidget::HandleOutlineToggled);
        Toolbar->OnEntityTypeFilterChanged.AddDynamic(this, &URootHUDWidget::HandleEntityTypeFilterChanged);
    }

    return true;
}

void URootHUDWidget::HandleOutlineToggled()
{
    if (OutlinePanel)
        OutlinePanel->TogglePanel();
}

void URootHUDWidget::HandleEntityTypeFilterChanged(const FString& TypeKey)
{
    // Forward to OutlinePanel / gamemode once built.
}

void URootHUDWidget::HandleEntitySelected(AActor* Actor)
{
    ATempUIActor* Entity = Cast<ATempUIActor>(Actor);
    if (!IsValid(Entity))
        return;

    SpawnOrFocusWindow(Entity);
}

void URootHUDWidget::SpawnOrFocusWindow(ATempUIActor* Actor)
{
    if (!EntityWindowContainer || !EntityWindowClass)
        return;

    const FString& ThingId = Actor->GetThingId();

    // If already open, bring to front.
    if (TWeakObjectPtr<UEntityWindowWidget>* Existing = OpenWindows.Find(ThingId))
    {
        if (Existing->IsValid())
        {
            if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Existing->Get()->Slot))
                CanvasSlot->SetZOrder(OpenWindows.Num());
            return;
        }
        OpenWindows.Remove(ThingId);
    }

    UEntityWindowWidget* Window = CreateWidget<UEntityWindowWidget>(GetOwningPlayer(), EntityWindowClass);
    if (!Window)
        return;

    Window->OnClosed.AddDynamic(this, &URootHUDWidget::HandleEntityWindowClosed);

    UCanvasPanelSlot* CanvasSlot = EntityWindowContainer->AddChildToCanvas(Window);
    if (CanvasSlot)
    {
        // Stagger each new window by 30px so they don't perfectly overlap.
        const int32 Offset = OpenWindows.Num() * 30;
        CanvasSlot->SetPosition(FVector2D(50.f + Offset, 150.f + Offset));
        CanvasSlot->SetAutoSize(true);
        CanvasSlot->SetZOrder(OpenWindows.Num());
    }

    OpenWindows.Add(ThingId, Window);
    Window->OpenForActor(Actor);
}

void URootHUDWidget::HandleEntityWindowClosed(const FString& ThingId)
{
    OpenWindows.Remove(ThingId);
}
