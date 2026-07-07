#include "UI/ToolbarButtonWidget.h"

void UToolbarButtonWidget::SetLabel(const FText& Text)
{
    if (Label)
        Label->SetText(Text);
}

void UToolbarButtonWidget::SetIcon(UTexture2D* Texture)
{
    if (Icon && Texture)
        Icon->SetBrushFromTexture(Texture);
}
