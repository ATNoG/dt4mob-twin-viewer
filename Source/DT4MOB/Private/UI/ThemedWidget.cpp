#include "UI/ThemedWidget.h"
#include "UI/UIThemeBFL.h"
#include "UI/UIThemeSubsystem.h"

void UThemedWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UUIThemeSubsystem* Sub = UUIThemeBFL::GetUIThemeSubsystem(this);
    if (Sub)
    {
        Sub->OnThemeChanged.AddDynamic(this, &UThemedWidget::ApplyTheme);
        ApplyTheme(Sub->GetTheme());
    }
}
