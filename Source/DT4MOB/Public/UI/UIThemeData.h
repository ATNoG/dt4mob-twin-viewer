/**
 * @file
 * @brief Data asset type that holds every static colour of one UI theme.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UIThemeData.generated.h"

/**
 * @brief Colour palette for one UI theme. The project has one asset for dark mode and one
 *        for light mode.
 *
 * The assets live at /Game/Blueprints/UI/DA_ThemeDark and DA_ThemeLight and are assigned in
 * Project Settings -> Game -> UI Theme Settings (UUIThemeSettings, saved to
 * Config/DefaultGame.ini). UUIThemeSubsystem loads the one for the current mode, and
 * widgets derived from UThemedWidget read these colours in their ApplyTheme() override.
 * Widget C++ code takes every static colour from here instead of hardcoding FLinearColor
 * values.
 *
 * The default values in this class are the dark palette; each asset overrides them in the
 * editor. Switch modes at runtime with UUIThemeSubsystem::SetDarkMode().
 *
 * @ingroup ui
 */
UCLASS(BlueprintType)
class DT4MOB_API UUIThemeData : public UDataAsset
{
    GENERATED_BODY()

public:
    // --- Backgrounds ---

    /**
     * @brief Main window and popup background (entity window body, dropdown popups, idle
     *        dropdown options). The options window uses it at 80% opacity as its backdrop.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor BackgroundPrimary   = FLinearColor::FromSRGBColor(FColor(0x1D, 0x1D, 0x1D, 0xFF));

    /** @brief Darkest backdrop, below BackgroundPrimary. Used behind the login screen. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor BackgroundDeepest   = FLinearColor::FromSRGBColor(FColor(0x08, 0x08, 0x08, 0xFF));

    /**
     * @brief Background of secondary controls inside a panel: info tab field borders, the
     *        info config Save/Reset buttons, the JSON tab and model group rows.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor BackgroundSecondary = FLinearColor(0.08f,  0.08f,  0.08f,  1.f);

    /**
     * @brief Toolbar strip, panel headers, login input fields and the active tab or options
     *        section.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor PanelBackground     = FLinearColor::FromSRGBColor(FColor(0x23, 0x23, 0x23, 0xFF));

    /** @brief Search box background in the outline panel. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor SearchBackground    = FLinearColor::FromSRGBColor(FColor(0x16, 0x16, 0x16, 0xFF));

    /** @brief Background of the outline panel's scrollable list body. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor ListBackground      = FLinearColor::FromSRGBColor(FColor(0x1D, 0x1D, 0x1D, 0xFF));

    /** @brief Background of even-numbered rows in the outline and association lists. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor RowBackgroundEven   = FLinearColor::FromSRGBColor(FColor(0x21, 0x21, 0x21, 0xFF));

    /** @brief Background of inactive tabs in the entity window and options window. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor TabInactiveBackground = FLinearColor::FromSRGBColor(FColor(0x18, 0x18, 0x18, 0xFF));

    /**
     * @brief Frame colour of window-style panels (entity, options and login windows) and the
     *        outline drawn around search and login input fields.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor WindowOutline       = FLinearColor::FromSRGBColor(FColor(0x12, 0x12, 0x12, 0xFF));

    // --- Buttons ---

    /**
     * @brief Hover tint for buttons, dropdown options and info fields. The default is a
     *        translucent white overlay. Toolbar buttons also use it for their active state.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    FLinearColor Hover               = FLinearColor(1.f, 1.f, 1.f, 0.06f);

    /** @brief Pressed tint for the same controls as Hover; the default is a stronger overlay. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    FLinearColor Pressed             = FLinearColor(1.f, 1.f, 1.f, 0.12f);

    /**
     * @brief Idle button background. Lighter than PanelBackground so buttons stand out from
     *        the toolbar.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    FLinearColor ButtonIdle          = FLinearColor::FromSRGBColor(FColor(0x38, 0x38, 0x38, 0xFF));

    /** @brief Hover background of outline list rows. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    FLinearColor RowHover            = FLinearColor::FromSRGBColor(FColor(0x2E, 0x2E, 0x2E, 0xFF));

    // --- Text ---

    /** @brief Main text colour: titles, labels, field values, active tab text. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
    FLinearColor TextPrimary         = FLinearColor::FromSRGBColor(FColor(0xDC, 0xDC, 0xDC, 0xFF));

    /**
     * @brief Dimmed text: field labels, thing ID subtitles, inactive tab text and the
     *        overview line in the outline panel.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
    FLinearColor TextSecondary       = FLinearColor::FromSRGBColor(FColor(0x7A, 0x7A, 0x7A, 0xFF));

    /** @brief Colour of the "Active" status label in the entity window. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
    FLinearColor StatusActive        = FLinearColor::FromSRGBColor(FColor(0x56, 0xCB, 0x78, 0xFF));

    /** @brief Error message text, such as login failures. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
    FLinearColor StatusError         = FLinearColor::FromSRGBColor(FColor(0xE0, 0x5C, 0x5C, 0xFF));

    /** @brief Text and arrow icon of the "Open in Grafana" link in the entity window. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
    FLinearColor GrafanaLink         = FLinearColor::FromSRGBColor(FColor(0xF4, 0xA4, 0x60, 0xFF));

    /** @brief Tint applied to toolbar button icons. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
    FLinearColor IconTint            = FLinearColor::FromSRGBColor(FColor(0xBF, 0xBF, 0xBF, 0xFF));

    // --- Accents ---

    /** @brief General accent colour (orange in the default palette). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accents")
    FLinearColor Accent              = FLinearColor(1.f, 0.4f, 0.13f, 1.f);

    /**
     * @brief Thin separator lines in the toolbar, entity window and options window. The
     *        login screen also tints its background pattern with it.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accents")
    FLinearColor Separator           = FLinearColor(1.f, 1.f, 1.f, 0.08f);

    /** @brief Translucent overlay for a selected item. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accents")
    FLinearColor SelectionHighlight  = FLinearColor(1.f, 1.f, 1.f, 0.1f);

    /** @brief Secondary highlight accent (light blue in the default palette). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accents")
    FLinearColor HighlightAccent     = FLinearColor(0.557f, 0.714f, 1.f, 1.f);

    /** @brief Text or indicator colour for unselected tabs. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accents")
    FLinearColor TabUnselected       = FLinearColor(0.45f, 0.45f, 0.45f, 1.f);
};
