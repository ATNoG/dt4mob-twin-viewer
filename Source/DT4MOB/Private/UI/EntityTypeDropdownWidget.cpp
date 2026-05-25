#include "UI/EntityTypeDropdownWidget.h"

bool UEntityTypeDropdownWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (DropdownButton)
        DropdownButton->OnClicked.AddDynamic(this, &UEntityTypeDropdownWidget::HandleDropdownButtonClicked);

    if (SelectedTypeText)
        SelectedTypeText->SetText(FText::FromString(TEXT("None")));

    return true;
}

void UEntityTypeDropdownWidget::PopulateTypes_Implementation(const TArray<FString>& TypeKeys)
{
    AvailableTypes = TypeKeys;
}

void UEntityTypeDropdownWidget::SelectType_Implementation(const FString& TypeKey)
{
    SelectedType = TypeKey;

    const FString DisplayText = TypeKey.IsEmpty() ? TEXT("None") : TypeKey;
    if (SelectedTypeText)
        SelectedTypeText->SetText(FText::FromString(DisplayText));

    CloseDropdown();
    OnTypeSelected.Broadcast(TypeKey);
}

void UEntityTypeDropdownWidget::ToggleDropdown_Implementation()
{
    bIsOpen = !bIsOpen;
    OnDropdownStateChanged.Broadcast(bIsOpen);
}

void UEntityTypeDropdownWidget::CloseDropdown_Implementation()
{
    bIsOpen = false;
    OnDropdownStateChanged.Broadcast(false);
}

void UEntityTypeDropdownWidget::HandleDropdownButtonClicked()
{
    ToggleDropdown();
}
