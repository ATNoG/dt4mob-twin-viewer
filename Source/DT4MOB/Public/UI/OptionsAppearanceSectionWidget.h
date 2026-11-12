/**
 * @file
 * @brief Declares UOptionsAppearanceSectionWidget, the theme picker section of the Options window.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "OptionsAppearanceSectionWidget.generated.h"

class UButton;
class UTextBlock;
class UBorder;
class UMenuAnchor;
class UUserWidget;
class UThemeDropdownPopupWidget;

/**
 * @brief "Appearance" section of the Options window: a dropdown that selects the Dark or Light theme.
 *
 * The choice maps to UUIThemeSubsystem::SetDarkMode(), so only the two themes configured in
 * UUIThemeSettings (DarkTheme and LightTheme) exist. It is presented as a dropdown rather than a
 * toggle so it reads as "select a theme" and has room for more themes later.
 *
 * The popup is driven by a UMenuAnchor that wraps AppearanceDropdownButton, so Slate places it
 * next to the button wherever the button is on screen. The popup content is a
 * UThemeDropdownPopupWidget created on demand by HandleGetMenuContent().
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UOptionsAppearanceSectionWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Binds the dropdown button events, strips the button's default brushes and hooks
     *        HandleGetMenuContent() and HandleAnchorOpenChanged() into AppearanceDropdownAnchor.
     * @return False if the base UUserWidget::Initialize() fails, true otherwise.
     */
    virtual bool Initialize() override;

    /** @brief Sets AppearanceValueLabel to "Dark" or "Light" from UUIThemeSubsystem::IsDarkMode(). UOptionsWindowWidget::Open() calls it. */
    void Refresh();

protected:
    /** @brief Dropdown header button that opens the theme popup. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* AppearanceDropdownButton;

    /** @brief Shows the active theme name ("Dark" or "Light"). Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* AppearanceValueLabel;

    /** @brief Row caption next to the dropdown ("Application Theme"). Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* AppearanceToggleLabel;

    /** @brief Border drawn behind AppearanceDropdownButton; shows the hover color while the popup is open. Optional. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* AppearanceDropdownBackground;

    /**
     * @brief Menu anchor that owns the theme popup and positions it next to AppearanceDropdownButton.
     *
     * Optional, but without it the dropdown does not open. In the widget Blueprint it must be
     * named "AppearanceDropdownAnchor" and hold AppearanceDropdownButton as its single content child.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UMenuAnchor* AppearanceDropdownAnchor;

    /** @brief Widget Blueprint class (derived from UThemeDropdownPopupWidget) created as the popup. Set in the Blueprint defaults; if unset, the bare C++ class is used. */
    UPROPERTY(EditDefaultsOnly, Category = "Appearance")
    TSubclassOf<UThemeDropdownPopupWidget> PopupWidgetClass;

    /**
     * @brief Colors the labels and dropdown background from Theme, then calls Refresh().
     *
     * Picking a theme in the popup broadcasts OnThemeChanged, which lands here, so this is also
     * what keeps AppearanceValueLabel in sync after a selection.
     *
     * @param Theme  Active theme; nothing changes if it is null.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    /**
     * @brief Sets the fill tint and the outline color of a background border.
     * @param Border  Border to recolor; ignored if null.
     * @param Color   New fill and outline color.
     */
    void SetOptionBackgroundColor(UBorder* Border, const FLinearColor& Color);

    /** @brief Dropdown background at rest (theme ButtonIdle). */
    FLinearColor OptionNormalColor = FLinearColor::Transparent;
    /** @brief Dropdown background when hovered or while the popup is open (theme Hover). */
    FLinearColor OptionHoverColor = FLinearColor::Transparent;
    /** @brief Dropdown background while pressed (theme Pressed). */
    FLinearColor OptionPressedColor = FLinearColor::Transparent;

    /** @brief Opens or closes the popup through AppearanceDropdownAnchor. */
    UFUNCTION()
    void HandleDropdownClicked();

    /** @brief Shows the hover color on AppearanceDropdownBackground. */
    UFUNCTION()
    void HandleDropdownHovered();

    /** @brief Restores the normal color, or keeps the hover color while the popup is open. */
    UFUNCTION()
    void HandleDropdownUnhovered();

    /** @brief Shows the pressed color on AppearanceDropdownBackground. */
    UFUNCTION()
    void HandleDropdownPressed();

    /** @brief Puts AppearanceDropdownBackground back to the hover color. */
    UFUNCTION()
    void HandleDropdownReleased();

    /**
     * @brief Keeps the dropdown background highlighted while the popup is open.
     * @param bIsOpen  True when the popup has just opened, false when it closed.
     */
    UFUNCTION()
    void HandleAnchorOpenChanged(bool bIsOpen);

    /**
     * @brief Creates a new popup each time the menu anchor opens and gives it the anchor so it can close itself.
     * @return The new UThemeDropdownPopupWidget, or null if CreateWidget() failed.
     */
    UFUNCTION()
    UUserWidget* HandleGetMenuContent();
};
