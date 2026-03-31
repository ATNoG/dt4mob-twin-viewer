/** @file UIManager.cpp
 *  @brief Implementation of UUIManager. All logic documentation is in the header.
 */
#include "Managers/UIManager.h"
#include "Managers/SelectionManager.h"

void UUIManager::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    if (ULocalPlayer *LP = GetLocalPlayer())
    {
        if (USelectionManager *SelectionManager =
                LP->GetSubsystem<USelectionManager>())
        {
            SelectionManager->OnSelectedActorChanged.AddDynamic(
                this,
                &UUIManager::HandleSelectionChanged);
        }
    }
}

void UUIManager::HandleSelectionChanged(AActor *NewActor)
{
    SetEntityWindowVisible(NewActor != nullptr);
}

void UUIManager::SetEntityWindowVisible(bool bVisible)
{
    if (bEntityWindowVisible == bVisible)
        return;

    bEntityWindowVisible = bVisible;
    OnEntityWindowVisibilityChanged.Broadcast(bEntityWindowVisible);
    UE_LOG(LogTemp, Log, TEXT("Entity window visibility changed: %s"),
           bEntityWindowVisible ? TEXT("Visible") : TEXT("Hidden"));
}