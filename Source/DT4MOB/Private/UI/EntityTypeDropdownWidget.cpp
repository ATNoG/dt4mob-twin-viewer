#include "UI/EntityTypeDropdownWidget.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "Engine/GameInstance.h"

bool UEntityTypeDropdownWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (DropdownButton)
    {
        DropdownButton->OnClicked.AddDynamic(this, &UEntityTypeDropdownWidget::HandleDropdownButtonClicked);
        DropdownButton->OnHovered.AddDynamic(this, &UEntityTypeDropdownWidget::HandleDropdownButtonHovered);
        DropdownButton->OnUnhovered.AddDynamic(this, &UEntityTypeDropdownWidget::HandleDropdownButtonUnhovered);
        DropdownButton->OnPressed.AddDynamic(this, &UEntityTypeDropdownWidget::HandleDropdownButtonPressed);
    }

    if (SelectedTypeText)
        SelectedTypeText->SetText(FText::FromString(TEXT("None")));

    return true;
}

void UEntityTypeDropdownWidget::ApplyTheme_Implementation(UUIThemeData* Theme)
{
    if (!Theme) return;

    ButtonNormalColor = Theme->ButtonIdle;
    ButtonHoverColor = Theme->Hover;
    ButtonPressedColor = Theme->Pressed;

    RefreshButtonBackground();

    if (DropdownPopup)
        DropdownPopup->SetBrushColor(Theme->BackgroundPrimary);

    if (SelectedTypeText)
        SelectedTypeText->SetColorAndOpacity(FSlateColor(Theme->TextPrimary));

    if (Type)
        Type->SetColorAndOpacity(FSlateColor(Theme->TextSecondary));

    if (Arrow)
        Arrow->SetColorAndOpacity(Theme->TextSecondary);
}

void UEntityTypeDropdownWidget::RefreshButtonBackground()
{
    if (!ButtonBackground)
        return;

    // Stay hover-tinted while the popup is open, regardless of actual mouse hover state.
    ButtonBackground->SetBrushColor(bIsOpen ? ButtonHoverColor : ButtonNormalColor);
}

void UEntityTypeDropdownWidget::HandleDropdownButtonHovered()
{
    if (!bIsOpen && ButtonBackground)
        ButtonBackground->SetBrushColor(ButtonHoverColor);
}

void UEntityTypeDropdownWidget::HandleDropdownButtonUnhovered()
{
    if (!bIsOpen && ButtonBackground)
        ButtonBackground->SetBrushColor(ButtonNormalColor);
}

void UEntityTypeDropdownWidget::HandleDropdownButtonPressed()
{
    if (ButtonBackground)
        ButtonBackground->SetBrushColor(ButtonPressedColor);
}

void UEntityTypeDropdownWidget::PopulateTypes_Implementation(const TArray<FString>& TypeKeys)
{
    AvailableTypes = TypeKeys;

    if (!OptionList)
        return;

    OptionList->ClearChildren();

    AddOptionChild(FString()); // "None" is always the first entry.

    for (const FString& Key : TypeKeys)
        AddOptionChild(Key);
}

void UEntityTypeDropdownWidget::AddOptionChild(const FString& TypeKeyToAdd)
{
    if (!OptionList || !OptionClass)
        return;

    UDropdownOptionWidget* Option = CreateWidget<UDropdownOptionWidget>(GetOwningPlayer(), OptionClass);
    if (!Option)
        return;

    FString DisplayNameForOption;
    bool bNoServerHandlingForOption = false;
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UDT4MOBEntityFactory* Factory = GI->GetSubsystem<UDT4MOBEntityFactory>())
        {
            const FEntityTypeMeta Meta = Factory->GetMetaForKey(TypeKeyToAdd);
            DisplayNameForOption = Meta.DisplayName;
            bNoServerHandlingForOption = Meta.bNoServerHandling;
        }
    }

    Option->SetTypeKey(TypeKeyToAdd);
    Option->SetDisplayName(DisplayNameForOption);
    Option->SetNoServerHandling(!TypeKeyToAdd.IsEmpty() && bNoServerHandlingForOption);

    OptionList->AddChild(Option);
    RegisterOption(Option);
}

void UEntityTypeDropdownWidget::RegisterOption(UDropdownOptionWidget* Option)
{
    if (!Option)
        return;

    Option->OnOptionClicked.AddDynamic(this, &UEntityTypeDropdownWidget::HandleOptionClicked);
}

void UEntityTypeDropdownWidget::HandleOptionClicked(const FString& TypeKey, const FString& OptionDisplayName)
{
    SelectType(TypeKey, OptionDisplayName);
}

void UEntityTypeDropdownWidget::SelectType_Implementation(const FString& TypeKey, const FString& InDisplayName)
{
    SelectedType = TypeKey;

    const FString DisplayText = TypeKey.IsEmpty() ? TEXT("None") : (InDisplayName.IsEmpty() ? TypeKey : InDisplayName);
    if (SelectedTypeText)
        SelectedTypeText->SetText(FText::FromString(DisplayText));

    CloseDropdown();
    OnTypeSelected.Broadcast(TypeKey);
}

void UEntityTypeDropdownWidget::ToggleDropdown_Implementation()
{
    bIsOpen = !bIsOpen;

    if (DropdownPopup)
        DropdownPopup->SetVisibility(bIsOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    RefreshButtonBackground();
    OnDropdownStateChanged.Broadcast(bIsOpen);
}

void UEntityTypeDropdownWidget::CloseDropdown_Implementation()
{
    bIsOpen = false;

    if (DropdownPopup)
        DropdownPopup->SetVisibility(ESlateVisibility::Collapsed);

    RefreshButtonBackground();
    OnDropdownStateChanged.Broadcast(false);
}

void UEntityTypeDropdownWidget::HandleDropdownButtonClicked()
{
    ToggleDropdown();
}
