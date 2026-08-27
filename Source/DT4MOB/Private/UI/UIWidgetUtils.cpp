#include "UI/UIWidgetUtils.h"
#include "Components/Button.h"
#include "Styling/SlateTypes.h"

void UIWidgetUtils::StripDefaultButtonBrushes(UButton* Button)
{
    if (!Button)
        return;

    FSlateBrush Transparent;
    Transparent.DrawAs = ESlateBrushDrawType::NoDrawType;

    FButtonStyle Style = Button->GetStyle();
    Style.Normal = Transparent;
    Style.Hovered = Transparent;
    Style.Pressed = Transparent;
    Style.Disabled = Transparent;
    Button->SetStyle(Style);
}
