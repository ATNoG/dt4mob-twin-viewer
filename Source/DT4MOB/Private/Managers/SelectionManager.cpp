/** @file SelectionManager.cpp
 *  @brief Implementation of USelectionManager. All logic documentation is in the header.
 */
#include "Managers/SelectionManager.h"
#include "Entities/TempUIActor.h"

void USelectionManager::SetSelectedActor(AActor *NewActor)
{
    // cast to temp UI actor to check if its a entity, if not ignore and deselect
    if (NewActor && !(NewActor->IsA<ATempUIActor>()))
    {
        NewActor = nullptr; // Deselect if it's a UI element
    }

    // Deselecting when nothing is selected is a true no-op. But re-clicking the
    // already-selected actor must still broadcast — listeners like RootHUDWidget
    // use this to reopen a window the user closed without changing selection.
    if (SelectedActor == NewActor && !NewActor)
        return;

    SelectedActor = NewActor;
    OnSelectedActorChanged.Broadcast(SelectedActor);
}

void USelectionManager::SetHoveredActor(AActor *NewActor)
{
    if (NewActor && !(NewActor->IsA<ATempUIActor>()))
    {
        NewActor = nullptr; // Clear hover if it's a UI element
    }

    if (HoveredActor == NewActor)
        return;

    HoveredActor = NewActor;
    OnHoveredActorChanged.Broadcast(HoveredActor);
}