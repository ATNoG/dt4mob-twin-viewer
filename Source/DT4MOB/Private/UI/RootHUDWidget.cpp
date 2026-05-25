#include "UI/RootHUDWidget.h"
#include "Managers/UIManager.h"
#include "Kismet/GameplayStatics.h"

bool URootHUDWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (APlayerController* PC = GetOwningPlayer())
    {
        UIManager = PC->GetLocalPlayer()->GetSubsystem<UUIManager>();
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
    // OutlinePanel wiring goes here once the panel is built.
}

void URootHUDWidget::HandleEntityTypeFilterChanged(const FString& TypeKey)
{
    // Forward to OutlinePanel / gamemode once built.
}
