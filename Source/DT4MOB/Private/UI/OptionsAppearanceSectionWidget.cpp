#include "UI/OptionsAppearanceSectionWidget.h"
#include "UI/UIThemeSubsystem.h"
#include "UI/ThemeDropdownPopupWidget.h"
#include "UI/UIWidgetUtils.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/MenuAnchor.h"
#include "Styling/SlateTypes.h"

using UIWidgetUtils::StripDefaultButtonBrushes;

bool UOptionsAppearanceSectionWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (AppearanceDropdownButton)
    {
        AppearanceDropdownButton->OnClicked.AddDynamic(this, &UOptionsAppearanceSectionWidget::HandleDropdownClicked);
        AppearanceDropdownButton->OnHovered.AddDynamic(this, &UOptionsAppearanceSectionWidget::HandleDropdownHovered);
        AppearanceDropdownButton->OnUnhovered.AddDynamic(this, &UOptionsAppearanceSectionWidget::HandleDropdownUnhovered);
        AppearanceDropdownButton->OnPressed.AddDynamic(this, &UOptionsAppearanceSectionWidget::HandleDropdownPressed);
        AppearanceDropdownButton->OnReleased.AddDynamic(this, &UOptionsAppearanceSectionWidget::HandleDropdownReleased);
        StripDefaultButtonBrushes(AppearanceDropdownButton);
    }

    if (AppearanceDropdownAnchor)
    {
        AppearanceDropdownAnchor->OnGetUserMenuContentEvent.BindDynamic(this, &UOptionsAppearanceSectionWidget::HandleGetMenuContent);
        AppearanceDropdownAnchor->OnMenuOpenChanged.AddDynamic(this, &UOptionsAppearanceSectionWidget::HandleAnchorOpenChanged);
    }

    return true;
}

void UOptionsAppearanceSectionWidget::Refresh()
{
    if (!AppearanceValueLabel || !GetOwningPlayer() || !GetOwningPlayer()->GetLocalPlayer())
        return;

    if (UUIThemeSubsystem* ThemeSubsystem = GetOwningPlayer()->GetLocalPlayer()->GetSubsystem<UUIThemeSubsystem>())
        AppearanceValueLabel->SetText(FText::FromString(ThemeSubsystem->IsDarkMode() ? TEXT("Dark") : TEXT("Light")));
}

void UOptionsAppearanceSectionWidget::SetOptionBackgroundColor(UBorder* Border, const FLinearColor& Color)
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

void UOptionsAppearanceSectionWidget::HandleDropdownClicked()
{
    if (AppearanceDropdownAnchor)
        AppearanceDropdownAnchor->ToggleOpen(false);
}

void UOptionsAppearanceSectionWidget::HandleDropdownHovered()   { SetOptionBackgroundColor(AppearanceDropdownBackground, OptionHoverColor); }
void UOptionsAppearanceSectionWidget::HandleDropdownUnhovered() { SetOptionBackgroundColor(AppearanceDropdownBackground, (AppearanceDropdownAnchor && AppearanceDropdownAnchor->IsOpen()) ? OptionHoverColor : OptionNormalColor); }
void UOptionsAppearanceSectionWidget::HandleDropdownPressed()   { SetOptionBackgroundColor(AppearanceDropdownBackground, OptionPressedColor); }
void UOptionsAppearanceSectionWidget::HandleDropdownReleased()  { SetOptionBackgroundColor(AppearanceDropdownBackground, OptionHoverColor); }

void UOptionsAppearanceSectionWidget::HandleAnchorOpenChanged(bool bIsOpen)
{
    SetOptionBackgroundColor(AppearanceDropdownBackground, bIsOpen ? OptionHoverColor : OptionNormalColor);
}

UUserWidget* UOptionsAppearanceSectionWidget::HandleGetMenuContent()
{
    const TSubclassOf<UThemeDropdownPopupWidget> ClassToUse = PopupWidgetClass ? PopupWidgetClass : TSubclassOf<UThemeDropdownPopupWidget>(UThemeDropdownPopupWidget::StaticClass());

    UThemeDropdownPopupWidget* Popup = CreateWidget<UThemeDropdownPopupWidget>(GetOwningPlayer(), ClassToUse);
    if (Popup)
        Popup->SetOwningAnchor(AppearanceDropdownAnchor);

    return Popup;
}

void UOptionsAppearanceSectionWidget::ApplyTheme_Implementation(UUIThemeData* Theme)
{
    if (!Theme) return;

    if (AppearanceValueLabel)
        AppearanceValueLabel->SetColorAndOpacity(FSlateColor(Theme->TextPrimary));

    if (AppearanceToggleLabel)
        AppearanceToggleLabel->SetColorAndOpacity(FSlateColor(Theme->TextPrimary));

    OptionNormalColor = Theme->ButtonIdle;
    OptionHoverColor = Theme->Hover;
    OptionPressedColor = Theme->Pressed;
    SetOptionBackgroundColor(AppearanceDropdownBackground, (AppearanceDropdownAnchor && AppearanceDropdownAnchor->IsOpen()) ? OptionHoverColor : OptionNormalColor);

    // Selecting a theme from the popup broadcasts OnThemeChanged, which routes back here —
    // catch it to keep the value label in sync without the popup needing a reference back to us.
    Refresh();
}
