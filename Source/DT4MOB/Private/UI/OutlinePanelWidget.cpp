#include "UI/OutlinePanelWidget.h"
#include "Entities/TempUIActor.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "Services/ActorRegistryService.h"
#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/ScrollBox.h"
#include "Engine/GameInstance.h"

bool UOutlinePanelWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (CloseButton)
        CloseButton->OnClicked.AddDynamic(this, &UOutlinePanelWidget::HandleCloseClicked);

    if (SearchBox)
        SearchBox->OnTextChanged.AddDynamic(this, &UOutlinePanelWidget::HandleSearchChanged);

    return true;
}

void UOutlinePanelWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Registry)
        return;

    if (UGameInstance* GI = GetGameInstance())
        Registry = GI->GetSubsystem<UActorRegistryService>();

    if (Registry)
    {
        Registry->OnEntityRegistered.AddDynamic(this, &UOutlinePanelWidget::HandleEntityRegistered);
        Registry->OnEntityUnregistered.AddDynamic(this, &UOutlinePanelWidget::HandleEntityUnregistered);
    }
}

void UOutlinePanelWidget::TogglePanel()
{
    if (bIsOpen)
    {
        bIsOpen = false;
        PlayCloseAnimation();
    }
    else
    {
        bIsOpen = true;
        SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        PopulateAll();
        PlayOpenAnimation();
    }
}

void UOutlinePanelWidget::PopulateAll()
{
    if (!EntityListBox || !RowWidgetClass || !Registry)
        return;

    EntityListBox->ClearChildren();
    AllRows.Empty();

    for (ATempUIActor* Actor : Registry->GetAllActors())
        AddRow(Actor);

    if (SearchBox)
        HandleSearchChanged(SearchBox->GetText());
}

void UOutlinePanelWidget::AddRow(ATempUIActor* Actor)
{
    if (!IsValid(Actor) || !EntityListBox || !RowWidgetClass)
        return;

    UOutlineRowWidget* Row = CreateWidget<UOutlineRowWidget>(GetOwningPlayer(), RowWidgetClass);
    if (!Row)
        return;

    FString TypeKey;
    FString DisplayName;
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UDT4MOBEntityFactory* Factory = GI->GetSubsystem<UDT4MOBEntityFactory>())
        {
            TypeKey = Factory->GetTypeKeyForThingId(Actor->GetThingId());
            DisplayName = Factory->GetMetaForKey(TypeKey).DisplayName;
        }
    }

    Row->SetData(Actor->GetThingId(), TypeKey, DisplayName, Actor);
    Row->SetEvenRow(AllRows.Num() % 2 == 0);
    Row->OnRowSelected.AddDynamic(this, &UOutlinePanelWidget::HandleRowSelected);
    EntityListBox->AddChild(Row);
    AllRows.Add(Row);

    // Apply current search filter to new row
    if (SearchBox)
    {
        const FString Filter = SearchBox->GetText().ToString().ToLower();
        if (!Filter.IsEmpty() && !Actor->GetThingId().ToLower().Contains(Filter))
            Row->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UOutlinePanelWidget::HandleCloseClicked()
{
    if (bIsOpen)
        TogglePanel();
}

void UOutlinePanelWidget::HandleSearchChanged(const FText& Text)
{
    const FString Filter = Text.ToString().ToLower();
    for (UOutlineRowWidget* Row : AllRows)
    {
        if (!Row)
            continue;
        const bool bVisible = Filter.IsEmpty() || Row->GetThingId().ToLower().Contains(Filter);
        Row->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }
}

void UOutlinePanelWidget::HandleEntityRegistered(ATempUIActor* Actor)
{
    AddRow(Actor);
}

void UOutlinePanelWidget::HandleRowSelected(const FString& ThingId)
{
    if (UActorRegistryService* Reg = UActorRegistryService::Get(this))
        if (ATempUIActor* Actor = Reg->FindActor(ThingId))
            OnEntityOpenRequested.Broadcast(Actor);
}

void UOutlinePanelWidget::HandleEntityUnregistered(const FString& ThingId)
{
    for (int32 i = AllRows.Num() - 1; i >= 0; --i)
    {
        if (AllRows[i] && AllRows[i]->GetThingId() == ThingId)
        {
            AllRows[i]->RemoveFromParent();
            AllRows.RemoveAt(i);
            break;
        }
    }
}
