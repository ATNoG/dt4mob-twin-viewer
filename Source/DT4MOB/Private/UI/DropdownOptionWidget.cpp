#include "UI/DropdownOptionWidget.h"

bool UDropdownOptionWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (OptionButton)
    {
        OptionButton->OnClicked.AddDynamic(this, &UDropdownOptionWidget::HandleOptionButtonClicked);
        OptionButton->OnHovered.AddDynamic(this, &UDropdownOptionWidget::HandleOptionButtonHovered);
        OptionButton->OnUnhovered.AddDynamic(this, &UDropdownOptionWidget::HandleOptionButtonUnhovered);
    }

    return true;
}

void UDropdownOptionWidget::ApplyTheme_Implementation(UUIThemeData* Theme)
{
    if (!Theme) return;

    NormalColor = Theme->BackgroundPrimary;
    HoverColor = Theme->Hover;

    if (ButtonBackground)
        ButtonBackground->SetBrushColor(NormalColor);

    if (OptionLabel)
        OptionLabel->SetColorAndOpacity(FSlateColor(Theme->TextPrimary));
}

void UDropdownOptionWidget::HandleOptionButtonClicked()
{
    OnOptionClicked.Broadcast(TypeKey, DisplayName);
}

void UDropdownOptionWidget::HandleOptionButtonHovered()
{
    if (ButtonBackground)
        ButtonBackground->SetBrushColor(HoverColor);
}

void UDropdownOptionWidget::HandleOptionButtonUnhovered()
{
    if (ButtonBackground)
        ButtonBackground->SetBrushColor(NormalColor);
}

void UDropdownOptionWidget::SetTypeKey(const FString& InTypeKey)
{
    TypeKey = InTypeKey;
    const FString DisplayText = InTypeKey.IsEmpty() ? TEXT("None") : InTypeKey;
    if (OptionLabel)
        OptionLabel->SetText(FText::FromString(DisplayText));
}

void UDropdownOptionWidget::SetDisplayName(const FString& InDisplayName)
{
    if (OptionLabel)
        OptionLabel->SetText(FText::FromString(InDisplayName.IsEmpty() ? TEXT("None") : InDisplayName));
}

void UDropdownOptionWidget::SetNoServerHandling(bool bValue)
{
    bNoServerHandling = bValue;
    if (Warningimage)
        Warningimage->SetVisibility(bValue ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

