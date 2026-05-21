#include "UI/RootHUDWidget.h"
#include "UI/EntityWindowWidget.h"
#include "UI/ToolbarWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Components/CanvasPanelSlot.h"
#include "Entities/TempUIActor.h"

bool URootHUDWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (APlayerController *PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        SelectionSubsystem = PC->GetLocalPlayer()->GetSubsystem<USelectionManager>();
    }

    if (SelectionSubsystem)
    {
        SelectionSubsystem->OnSelectedActorChanged.AddDynamic(this, &URootHUDWidget::HandleSelectionChanged);
    }

    if (Toolbar)
    {
        Toolbar->OnOutlineToggled.AddDynamic(this, &URootHUDWidget::HandleOutlineToggled);
    }

    return true;
}

void URootHUDWidget::HandleSelectionChanged(AActor *SelectedActor)
{
    if (ATempUIActor *TempActor = Cast<ATempUIActor>(SelectedActor))
    {
        OpenWindowForActor(TempActor);
    }
}

void URootHUDWidget::OpenWindowForActor(ATempUIActor *Actor)
{
    if (!Actor || !WindowContainer || !EntityWindowClass)
        return;

    // Already open — bring to front by re-adding (moves to top of Z-order).
    if (UEntityWindowWidget **Existing = OpenWindows.Find(Actor))
    {
        (*Existing)->SetVisibility(ESlateVisibility::Visible);
        return;
    }

    UEntityWindowWidget *NewWindow = CreateWidget<UEntityWindowWidget>(GetOwningPlayer(), EntityWindowClass);
    if (!NewWindow)
        return;

    WindowContainer->AddChild(NewWindow);

    if (UCanvasPanelSlot *PanelSlot = Cast<UCanvasPanelSlot>(NewWindow->Slot))
    {
        // Stagger each new window slightly so they don't all stack exactly on top.
        const int32 Offset = OpenWindows.Num() * 30;
        PanelSlot->SetPosition(FVector2D(120.f + Offset, 120.f + Offset));
        PanelSlot->SetAutoSize(true);
    }

    NewWindow->SetOwnerHUD(this);
    NewWindow->InitCanvasSlot();
    NewWindow->OpenWindow();
    NewWindow->OnBindData(Actor);

    OpenWindows.Add(Actor, NewWindow);
}

void URootHUDWidget::CloseWindowForActor(ATempUIActor *Actor)
{
    if (!Actor)
        return;

    if (UEntityWindowWidget **Found = OpenWindows.Find(Actor))
    {
        (*Found)->CloseWindow();
        (*Found)->RemoveFromParent();
        OpenWindows.Remove(Actor);
    }
}

void URootHUDWidget::HandleOutlineToggled()
{
    // OutlinePanel wiring goes here once the panel widget is implemented.
}
