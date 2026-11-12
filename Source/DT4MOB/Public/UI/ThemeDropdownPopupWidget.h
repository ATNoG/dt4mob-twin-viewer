/**
 * @file
 * @brief Declares UThemeDropdownPopupWidget, the Dark/Light popup of the Appearance options.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "ThemeDropdownPopupWidget.generated.h"

class UButton;
class UTextBlock;
class UBorder;
class UMenuAnchor;

/**
 * @brief Popup content for the Appearance dropdown (Dark/Light).
 *
 * Instantiated fresh each time the dropdown opens, via
 * UOptionsAppearanceSectionWidget's AppearanceDropdownAnchor (a UMenuAnchor) binding
 * OnGetUserMenuContentEvent in C++: see OptionsAppearanceSectionWidget::HandleGetMenuContent.
 * MenuAnchor requires popup content to be a UUserWidget, so this can't just be the plain
 * Border it used to be; SetOwningAnchor() gives it a way to close itself after a pick.
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UThemeDropdownPopupWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Strips the default button brushes and binds the option buttons.
     * @return False if the base widget failed to initialise.
     */
    virtual bool Initialize() override;

    /**
     * @brief Sets the menu anchor to close after an option is picked.
     * Call right after CreateWidget, before returning this from OnGetUserMenuContentEvent.
     * @param InAnchor Anchor that opened this popup.
     */
    void SetOwningAnchor(UMenuAnchor* InAnchor);

protected:
    /** @brief Popup panel background. Must be named "PopupBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* PopupBackground;

    /** @brief Selects the dark theme. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* DarkOptionButton;

    /** @brief Optional label of the dark option. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* DarkOptionLabel;

    /** @brief Background pill behind DarkOptionButton. Must be named "DarkOptionBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* DarkOptionBackground;

    /** @brief Selects the light theme. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* LightOptionButton;

    /** @brief Optional label of the light option. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* LightOptionLabel;

    /** @brief Background pill behind LightOptionButton. Must be named "LightOptionBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* LightOptionBackground;

    /**
     * @brief Colours the popup background, the option labels and the option pills from the theme.
     * @param Theme Active theme.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    /** @brief Anchor that opened the popup; closed after a pick. */
    TWeakObjectPtr<UMenuAnchor> OwningAnchor;

    FLinearColor OptionNormalColor = FLinearColor::Transparent;  ///< Option pill colour at rest, from the theme.
    FLinearColor OptionHoverColor = FLinearColor::Transparent;   ///< Option pill colour on hover, from the theme.
    FLinearColor OptionPressedColor = FLinearColor::Transparent; ///< Option pill colour while pressed, from the theme.

    /**
     * @brief Switches the theme through UUIThemeSubsystem and closes the popup.
     * @param bDarkMode True for the dark theme, false for the light theme.
     */
    void SelectDarkMode(bool bDarkMode);

    /**
     * @brief Tints a border's brush and its outline with one colour, keeping the brush shape from the Designer.
     * @param Border Border to tint. Ignored if null.
     * @param Color  New colour.
     */
    void SetOptionBackgroundColor(UBorder* Border, const FLinearColor& Color);

    UFUNCTION()
    void HandleDarkOptionClicked();    ///< Selects the dark theme.

    UFUNCTION()
    void HandleLightOptionClicked();   ///< Selects the light theme.

    UFUNCTION()
    void HandleDarkOptionHovered();    ///< Sets the dark pill to the hover colour.

    UFUNCTION()
    void HandleDarkOptionUnhovered();  ///< Sets the dark pill back to the normal colour.

    UFUNCTION()
    void HandleDarkOptionPressed();    ///< Sets the dark pill to the pressed colour.

    UFUNCTION()
    void HandleDarkOptionReleased();   ///< Sets the dark pill back to the hover colour.

    UFUNCTION()
    void HandleLightOptionHovered();   ///< Sets the light pill to the hover colour.

    UFUNCTION()
    void HandleLightOptionUnhovered(); ///< Sets the light pill back to the normal colour.

    UFUNCTION()
    void HandleLightOptionPressed();   ///< Sets the light pill to the pressed colour.

    UFUNCTION()
    void HandleLightOptionReleased();  ///< Sets the light pill back to the hover colour.
};
