#include "UI/ToolbarButtonWidget.h"
#include "Styling/SlateTypes.h"

bool UToolbarButtonWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (Button)
    {
        Button->OnHovered.AddDynamic(this, &UToolbarButtonWidget::HandleButtonHovered);
        Button->OnUnhovered.AddDynamic(this, &UToolbarButtonWidget::HandleButtonUnhovered);
        Button->OnPressed.AddDynamic(this, &UToolbarButtonWidget::HandleButtonPressed);
        Button->OnReleased.AddDynamic(this, &UToolbarButtonWidget::HandleButtonReleased);

        // ButtonBackground (a separate Border) draws all the visible chrome — strip the
        // engine's default button brushes (the white outline) so only the border shows.
        FSlateBrush Transparent;
        Transparent.DrawAs = ESlateBrushDrawType::NoDrawType;

        FButtonStyle Style = Button->GetStyle();
        Style.Normal = Transparent;
        Style.Hovered = Transparent;
        Style.Pressed = Transparent;
        Style.Disabled = Transparent;
        Button->SetStyle(Style);
    }

    // Icon-only buttons (e.g. Options) leave Label's Designer-authored text empty — collapse it
    // so it doesn't still reserve layout space/padding next to the icon.
    if (Label && Label->GetText().IsEmpty())
        Label->SetVisibility(ESlateVisibility::Collapsed);

    return true;
}

void UToolbarButtonWidget::SetLabel(const FText& Text)
{
    if (!Label)
        return;

    Label->SetText(Text);
    Label->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void UToolbarButtonWidget::SetIcon(UTexture2D* Texture)
{
    if (Icon && Texture)
        Icon->SetBrushFromTexture(Texture);
}

void UToolbarButtonWidget::SetActiveState(bool bActive)
{
    bIsActive = bActive;
    RefreshButtonStyle();
}

void UToolbarButtonWidget::ApplyTheme_Implementation(UUIThemeData* Theme)
{
    if (!Theme) return;

    NormalColor = Theme->ButtonIdle;
    HoverColor = Theme->Hover;
    PressedColor = Theme->Pressed;
    ActiveColor = Theme->Hover;

    RefreshButtonStyle();

    if (Label)
        Label->SetColorAndOpacity(FSlateColor(Theme->TextPrimary));

    if (Icon)
        Icon->SetColorAndOpacity(Theme->IconTint);
}

void UToolbarButtonWidget::SetBackgroundColor(const FLinearColor& Color)
{
    if (!ButtonBackground)
        return;

    // Preserve whatever the Designer-authored brush looks like (shape, corner radius),
    // but match the outline color to the fill — the Designer brush bakes in a white
    // OutlineSettings.Color that otherwise shows through regardless of the tint we apply.
    FSlateBrush Brush = ButtonBackground->Background;
    Brush.TintColor = FSlateColor(Color);
    Brush.OutlineSettings.Color = FSlateColor(Color);
    ButtonBackground->SetBrush(Brush);
}

void UToolbarButtonWidget::RefreshButtonStyle()
{
    SetBackgroundColor(bIsActive ? ActiveColor : NormalColor);
}

void UToolbarButtonWidget::HandleButtonHovered()
{
    SetBackgroundColor(HoverColor);
}

void UToolbarButtonWidget::HandleButtonUnhovered()
{
    RefreshButtonStyle();
}

void UToolbarButtonWidget::HandleButtonPressed()
{
    SetBackgroundColor(PressedColor);
}

void UToolbarButtonWidget::HandleButtonReleased()
{
    SetBackgroundColor(HoverColor);
}
