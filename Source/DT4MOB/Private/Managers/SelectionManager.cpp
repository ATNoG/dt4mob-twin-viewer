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

    if (SelectedActor == NewActor)
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