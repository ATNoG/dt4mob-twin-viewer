#include "UI/ThemeDropdownPopupWidget.h"
#include "UI/UIThemeSubsystem.h"
#include "UI/UIWidgetUtils.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/MenuAnchor.h"
#include "Styling/SlateTypes.h"

using UIWidgetUtils::StripDefaultButtonBrushes;

bool UThemeDropdownPopupWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (DarkOptionButton)
    {
        DarkOptionButton->OnClicked.AddDynamic(this, &UThemeDropdownPopupWidget::HandleDarkOptionClicked);
        DarkOptionButton->OnHovered.AddDynamic(this, &UThemeDropdownPopupWidget::HandleDarkOptionHovered);
        DarkOptionButton->OnUnhovered.AddDynamic(this, &UThemeDropdownPopupWidget::HandleDarkOptionUnhovered);
        DarkOptionButton->OnPressed.AddDynamic(this, &UThemeDropdownPopupWidget::HandleDarkOptionPressed);
        DarkOptionButton->OnReleased.AddDynamic(this, &UThemeDropdownPopupWidget::HandleDarkOptionReleased);
        StripDefaultButtonBrushes(DarkOptionButton);
    }

    if (LightOptionButton)
    {
        LightOptionButton->OnClicked.AddDynamic(this, &UThemeDropdownPopupWidget::HandleLightOptionClicked);
        LightOptionButton->OnHovered.AddDynamic(this, &UThemeDropdownPopupWidget::HandleLightOptionHovered);
        LightOptionButton->OnUnhovered.AddDynamic(this, &UThemeDropdownPopupWidget::HandleLightOptionUnhovered);
        LightOptionButton->OnPressed.AddDynamic(this, &UThemeDropdownPopupWidget::HandleLightOptionPressed);
        LightOptionButton->OnReleased.AddDynamic(this, &UThemeDropdownPopupWidget::HandleLightOptionReleased);
        StripDefaultButtonBrushes(LightOptionButton);
    }

    return true;
}

void UThemeDropdownPopupWidget::SetOwningAnchor(UMenuAnchor* InAnchor)
{
    OwningAnchor = InAnchor;
}

void UThemeDropdownPopupWidget::SelectDarkMode(bool bDarkMode)
{
    if (GetOwningPlayer() && GetOwningPlayer()->GetLocalPlayer())
    {
        if (UUIThemeSubsystem* ThemeSubsystem = GetOwningPlayer()->GetLocalPlayer()->GetSubsystem<UUIThemeSubsystem>())
            ThemeSubsystem->SetDarkMode(bDarkMode);
    }

    if (OwningAnchor.IsValid())
        OwningAnchor->Close();
}

void UThemeDropdownPopupWidget::SetOptionBackgroundColor(UBorder* Border, const FLinearColor& Color)
{
    if (!Border)
        return;

    // Preserve whatever the Designer-authored brush looks like (shape, corner radius),
    // but match the outline color to the fill — the Designer brush bakes in a white
    // OutlineSettings.Color that otherwise shows through regardless of the tint we apply.
    FSlateBrush Brush = Border->Background;
    Brush.TintColor = FSlateColor(Color);
    Brush.OutlineSettings.Color = FSlateColor(Color);
    Border->SetBrush(Brush);
}

void UThemeDropdownPopupWidget::HandleDarkOptionClicked()  { SelectDarkMode(true); }
void UThemeDropdownPopupWidget::HandleLightOptionClicked() { SelectDarkMode(false); }

void UThemeDropdownPopupWidget::HandleDarkOptionHovered()   { SetOptionBackgroundColor(DarkOptionBackground, OptionHoverColor); }
void UThemeDropdownPopupWidget::HandleDarkOptionUnhovered() { SetOptionBackgroundColor(DarkOptionBackground, OptionNormalColor); }
void UThemeDropdownPopupWidget::HandleDarkOptionPressed()   { SetOptionBackgroundColor(DarkOptionBackground, OptionPressedColor); }
void UThemeDropdownPopupWidget::HandleDarkOptionReleased()  { SetOptionBackgroundColor(DarkOptionBackground, OptionHoverColor); }

void UThemeDropdownPopupWidget::HandleLightOptionHovered()   { SetOptionBackgroundColor(LightOptionBackground, OptionHoverColor); }
void UThemeDropdownPopupWidget::HandleLightOptionUnhovered() { SetOptionBackgroundColor(LightOptionBackground, OptionNormalColor); }
void UThemeDropdownPopupWidget::HandleLightOptionPressed()   { SetOptionBackgroundColor(LightOptionBackground, OptionPressedColor); }
void UThemeDropdownPopupWidget::HandleLightOptionReleased()  { SetOptionBackgroundColor(LightOptionBackground, OptionHoverColor); }

void UThemeDropdownPopupWidget::ApplyTheme_Implementation(UUIThemeData* Theme)
{
    if (!Theme) return;

    SetOptionBackgroundColor(PopupBackground, Theme->PanelBackground);

    if (DarkOptionLabel)
        DarkOptionLabel->SetColorAndOpacity(FSlateColor(Theme->TextPrimary));

    if (LightOptionLabel)
        LightOptionLabel->SetColorAndOpacity(FSlateColor(Theme->TextPrimary));

    OptionNormalColor = Theme->ButtonIdle;
    OptionHoverColor = Theme->Hover;
    OptionPressedColor = Theme->Pressed;
    SetOptionBackgroundColor(DarkOptionBackground, OptionNormalColor);
    SetOptionBackgroundColor(LightOptionBackground, OptionNormalColor);
}
