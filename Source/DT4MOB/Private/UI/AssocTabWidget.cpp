#include "UI/AssocTabWidget.h"
#include "Entities/TempUIActor.h"
#include "Services/ActorRegistryService.h"
#include "Managers/SelectionManager.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

void UAssocTabWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (UActorRegistryService* Reg = UActorRegistryService::Get(this))
    {
        Reg->OnEntityRegistered.AddDynamic(this, &UAssocTabWidget::HandleEntityRegistered);
        Reg->OnEntityUnregistered.AddDynamic(this, &UAssocTabWidget::HandleEntityUnregistered);
    }
}

void UAssocTabWidget::NativeDestruct()
{
    if (UActorRegistryService* Reg = UActorRegistryService::Get(this))
    {
        Reg->OnEntityRegistered.RemoveDynamic(this, &UAssocTabWidget::HandleEntityRegistered);
        Reg->OnEntityUnregistered.RemoveDynamic(this, &UAssocTabWidget::HandleEntityUnregistered);
    }

    BoundActor = nullptr;
    Super::NativeDestruct();
}

// ── Public API ────────────────────────────────────────────────────────────────

void UAssocTabWidget::SetBoundActor(ATempUIActor* Actor)
{
    BoundActor = Actor;
    RebuildList();
}

// ── List building ─────────────────────────────────────────────────────────────

void UAssocTabWidget::RebuildList()
{
    if (!AssocList)
        return;

    AssocList->ClearChildren();

    if (!IsValid(BoundActor) || !RowClass)
    {
        if (CountLabel)
            CountLabel->SetText(FText::FromString(TEXT("ASSOCIATED (0)")));
        return;
    }

    UActorRegistryService* Registry = UActorRegistryService::Get(this);
    if (!Registry)
        return;

    const FString Prefix = BoundActor->GetThingId() + TEXT(".instrument.");
    TArray<ATempUIActor*> Associated = Registry->FindActorsWithPrefix(Prefix);

    if (CountLabel)
        CountLabel->SetText(FText::FromString(
            FString::Printf(TEXT("ASSOCIATED (%d)"), Associated.Num())));

    for (int32 i = 0; i < Associated.Num(); i++)
    {
        ATempUIActor* Actor = Associated[i];
        if (!IsValid(Actor))
            continue;

        UAssocRowWidget* Row = CreateWidget<UAssocRowWidget>(GetOwningPlayer(), RowClass);
        if (!Row)
            continue;

        Row->SetActor(Actor);
        Row->SetEvenRow(i % 2 == 0);
        Row->OnOpenRequested.AddDynamic(this, &UAssocTabWidget::HandleOpenRequested);
        if (UVerticalBoxSlot* RowSlot = AssocList->AddChildToVerticalBox(Row))
        {
            RowSlot->SetPadding(FMargin(0.f));
            RowSlot->SetHorizontalAlignment(HAlign_Fill);
        }
    }
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void UAssocTabWidget::HandleOpenRequested(const FString& ThingId)
{
    UActorRegistryService* Registry = UActorRegistryService::Get(this);
    if (!Registry)
        return;

    ATempUIActor* Actor = Registry->FindActor(ThingId);
    if (!IsValid(Actor))
        return;

    if (APlayerController* PC = GetOwningPlayer())
        if (USelectionManager* SM = PC->GetLocalPlayer()->GetSubsystem<USelectionManager>())
            SM->SetSelectedActor(Actor);
}

void UAssocTabWidget::HandleEntityRegistered(ATempUIActor* Actor)
{
    if (!IsValid(BoundActor) || !IsValid(Actor))
        return;

    if (Actor->GetThingId().StartsWith(BoundActor->GetThingId() + TEXT(".instrument.")))
        RebuildList();
}

void UAssocTabWidget::HandleEntityUnregistered(const FString& ThingId)
{
    if (!IsValid(BoundActor))
        return;

    if (ThingId.StartsWith(BoundActor->GetThingId() + TEXT(".instrument.")))
        RebuildList();
}
