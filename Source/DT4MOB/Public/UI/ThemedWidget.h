#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/UIThemeData.h"
#include "ThemedWidget.generated.h"

/**
 * Base class for any widget that needs theme colours.
 * Automatically binds to UIThemeSubsystem on construct and calls ApplyTheme()
 * whenever the theme changes. Blueprints just implement ApplyTheme.
 */
UCLASS()
class DT4MOB_API UThemedWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    virtual void NativeConstruct() override;

    /** Called on construct and whenever the theme changes. Override in Blueprint to apply colours. */
    UFUNCTION(BlueprintImplementableEvent, Category = "UI|Theme")
    void ApplyTheme(UUIThemeData* Theme);
};
