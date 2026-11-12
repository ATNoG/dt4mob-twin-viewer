/**
 * @file
 * @brief Declares UThemedWidget, the base class for widgets that take their colors from the UI theme.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/UIThemeData.h"
#include "ThemedWidget.generated.h"

/**
 * @brief Base class for widgets that color themselves from the active UUIThemeData.
 *
 * NativeConstruct() binds ApplyTheme() to UUIThemeSubsystem::OnThemeChanged and calls it once with
 * the current theme. Subclasses only override ApplyTheme_Implementation() (or implement the
 * ApplyTheme event in Blueprint) to push theme colors onto their child widgets.
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UThemedWidget : public UUserWidget
{
    GENERATED_BODY()

protected:
    /** @brief Subscribes to UUIThemeSubsystem::OnThemeChanged and applies the current theme right away. Does nothing if the subsystem is not found. */
    virtual void NativeConstruct() override;

    /**
     * @brief Applies the colors of Theme to this widget's children.
     *
     * Runs from NativeConstruct() and again on every UUIThemeSubsystem::OnThemeChanged broadcast.
     * Override ApplyTheme_Implementation() in C++ or implement the event in Blueprint.
     *
     * @param Theme  Active theme asset. The C++ overrides in this module return early when it is null.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "UI|Theme")
    void ApplyTheme(UUIThemeData* Theme);

    /**
     * @brief Default implementation of ApplyTheme(); does nothing.
     * @param Theme  Active theme asset.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) {}
};
