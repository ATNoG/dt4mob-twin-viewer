#include "UI/OptionsWindowWidget.h"
#include "UI/OptionsAppearanceSectionWidget.h"
#include "UI/OptionsGeneralSectionWidget.h"
#include "UI/UIWidgetUtils.h"
#include "Gameplay/UnifiedPawn/UnifiedController.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Styling/SlateTypes.h"

using UIWidgetUtils::StripDefaultButtonBrushes;

bool UOptionsWindowWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (CloseButton)
        CloseButton->OnClicked.AddDynamic(this, &UOptionsWindowWidget::HandleCloseClicked);

    if (AppearanceSectionButton)
        AppearanceSectionButton->OnClicked.AddDynamic(this, &UOptionsWindowWidget::HandleAppearanceSectionClicked);

    if (GeneralSectionButton)
        GeneralSectionButton->OnClicked.AddDynamic(this, &UOptionsWindowWidget::HandleGeneralSectionClicked);

    if (LogoutButton)
    {
        LogoutButton->OnClicked.AddDynamic(this, &UOptionsWindowWidget::HandleLogoutClicked);
        LogoutButton->OnHovered.AddDynamic(this, &UOptionsWindowWidget::HandleLogoutHovered);
        LogoutButton->OnUnhovered.AddDynamic(this, &UOptionsWindowWidget::HandleLogoutUnhovered);
        LogoutButton->OnPressed.AddDynamic(this, &UOptionsWindowWidget::HandleLogoutPressed);
        LogoutButton->OnReleased.AddDynamic(this, &UOptionsWindowWidget::HandleLogoutReleased);
        StripDefaultButtonBrushes(LogoutButton);
    }

    if (ExitButton)
    {
        ExitButton->OnClicked.AddDynamic(this, &UOptionsWindowWidget::HandleExitClicked);
        ExitButton->OnHovered.AddDynamic(this, &UOptionsWindowWidget::HandleExitHovered);
        ExitButton->OnUnhovered.AddDynamic(this, &UOptionsWindowWidget::HandleExitUnhovered);
        ExitButton->OnPressed.AddDynamic(this, &UOptionsWindowWidget::HandleExitPressed);
        ExitButton->OnReleased.AddDynamic(this, &UOptionsWindowWidget::HandleExitReleased);
        StripDefaultButtonBrushes(ExitButton);
    }

    SetVisibility(ESlateVisibility::Collapsed);

    return true;
}

void UOptionsWindowWidget::Open()
{
    SwitchToSection(0);

    if (AppearanceSection)
        AppearanceSection->Refresh();

    SetVisibility(ESlateVisibility::Visible);
}

void UOptionsWindowWidget::Close()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

void UOptionsWindowWidget::SwitchToSection(int32 Index)
{
    ActiveSectionIndex = Index;

    if (SectionSwitcher)
        SectionSwitcher->SetActiveWidgetIndex(Index);

    RefreshSectionHighlight();
}

void UOptionsWindowWidget::RefreshSectionHighlight()
{
    if (AppearanceSectionBorder)
        AppearanceSectionBorder->SetBrushColor(ActiveSectionIndex == 1 ? SectionActiveColor : SectionInactiveColor);

    if (GeneralSectionBorder)
        GeneralSectionBorder->SetBrushColor(ActiveSectionIndex == 0 ? SectionActiveColor : SectionInactiveColor);
}

void UOptionsWindowWidget::SetActionButtonColor(UBorder* Border, const FLinearColor& Color)
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

void UOptionsWindowWidget::HandleAppearanceSectionClicked() { SwitchToSection(1); }
void UOptionsWindowWidget::HandleGeneralSectionClicked()    { SwitchToSection(0); }

void UOptionsWindowWidget::HandleCloseClicked()
{
    Close();
}

void UOptionsWindowWidget::HandleLogoutClicked()
{
    if (AUnifiedController* Controller = Cast<AUnifiedController>(GetOwningPlayer()))
        Controller->Logout();
}

void UOptionsWindowWidget::HandleExitClicked()
{
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

void UOptionsWindowWidget::HandleLogoutHovered()   { SetActionButtonColor(LogoutBackground, ActionButtonHoverColor); }
void UOptionsWindowWidget::HandleLogoutUnhovered() { SetActionButtonColor(LogoutBackground, ActionButtonNormalColor); }
void UOptionsWindowWidget::HandleLogoutPressed()   { SetActionButtonColor(LogoutBackground, ActionButtonPressedColor); }
void UOptionsWindowWidget::HandleLogoutReleased()  { SetActionButtonColor(LogoutBackground, ActionButtonHoverColor); }

void UOptionsWindowWidget::HandleExitHovered()   { SetActionButtonColor(ExitBackground, ActionButtonHoverColor); }
void UOptionsWindowWidget::HandleExitUnhovered() { SetActionButtonColor(ExitBackground, ActionButtonNormalColor); }
void UOptionsWindowWidget::HandleExitPressed()   { SetActionButtonColor(ExitBackground, ActionButtonPressedColor); }
void UOptionsWindowWidget::HandleExitReleased()  { SetActionButtonColor(ExitBackground, ActionButtonHoverColor); }

void UOptionsWindowWidget::ApplyTheme_Implementation(UUIThemeData* Theme)
{
    if (!Theme) return;

    if (WidgetBorder)
    {
        FLinearColor BackdropColor = Theme->BackgroundPrimary;
        BackdropColor.A = 0.8f;
        WidgetBorder->SetBrushColor(BackdropColor);
    }

    if (WindowBorder)
        WindowBorder->SetBrushColor(Theme->WindowOutline);

    if (TitleLabel)
        TitleLabel->SetColorAndOpacity(FSlateColor(Theme->TextPrimary));

    if (AppearanceSectionLabel)
        AppearanceSectionLabel->SetColorAndOpacity(FSlateColor(Theme->TextPrimary));

    if (GeneralSectionLabel)
        GeneralSectionLabel->SetColorAndOpacity(FSlateColor(Theme->TextPrimary));

    if (LogoutLabel)
        LogoutLabel->SetColorAndOpacity(FSlateColor(Theme->TextPrimary));

    if (ExitLabel)
        ExitLabel->SetColorAndOpacity(FSlateColor(Theme->TextPrimary));

    if (Separator)
        Separator->SetBrushColor(Theme->Separator);

    if (Separator2)
        Separator2->SetBrushColor(Theme->Separator);

    SectionActiveColor = Theme->PanelBackground;
    SectionInactiveColor = Theme->TabInactiveBackground;
    RefreshSectionHighlight();

    ActionButtonNormalColor = Theme->ButtonIdle;
    ActionButtonHoverColor = Theme->Hover;
    ActionButtonPressedColor = Theme->Pressed;
    SetActionButtonColor(LogoutBackground, ActionButtonNormalColor);
    SetActionButtonColor(ExitBackground, ActionButtonNormalColor);
}
