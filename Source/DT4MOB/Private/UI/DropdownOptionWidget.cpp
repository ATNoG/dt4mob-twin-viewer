#include "UI/DropdownOptionWidget.h"

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

