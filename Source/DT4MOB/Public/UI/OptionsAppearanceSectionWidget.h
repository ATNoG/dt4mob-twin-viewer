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
 * @brief Options window "Appearance" section — a dropdown picking between the two themes
 * UUIThemeSettings supports (DarkTheme/LightTheme). Still a binary choice under the hood
 * (UUIThemeSubsystem::SetDarkMode(bool)); presented as a dropdown rather than a toggle so it
 * reads as "select a theme" and has room to grow if more themes are ever added.
 *
 * The dropdown popup is a UMenuAnchor (AppearanceDropdownButton is its anchor content) rather
 * than a manually shown/hidden Border, so it always positions itself relative to the button's
 * actual on-screen location — see UThemeDropdownPopupWidget for the popup content itself.
 */
UCLASS()
class DT4MOB_API UOptionsAppearanceSectionWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    /** Re-reads the current theme and refreshes the value label. Call when the section becomes visible. */
    void Refresh();

protected:
    /** Dropdown header ("Appearance  Dark  ▾"). Must be named "AppearanceDropdownButton" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* AppearanceDropdownButton;

    /** Shows the currently active theme name. Must be named "AppearanceValueLabel" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* AppearanceValueLabel;

    /** Row label ("Application Theme"). Must be named "AppearanceToggleLabel" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* AppearanceToggleLabel;

    /** Background pill behind AppearanceDropdownButton. Must be named "AppearanceDropdownBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* AppearanceDropdownBackground;

    /**
     * Wraps AppearanceDropdownButton so the popup is anchored to it and repositions
     * automatically if it moves. Must be named "AppearanceDropdownAnchor" in the Blueprint
     * layout, with AppearanceDropdownButton as its single Content child.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UMenuAnchor* AppearanceDropdownAnchor;

    /** Widget Blueprint class (derived from UThemeDropdownPopupWidget) to spawn as the popup. */
    UPROPERTY(EditDefaultsOnly, Category = "Appearance")
    TSubclassOf<UThemeDropdownPopupWidget> PopupWidgetClass;

    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    void SetOptionBackgroundColor(UBorder* Border, const FLinearColor& Color);

    FLinearColor OptionNormalColor = FLinearColor::Transparent;
    FLinearColor OptionHoverColor = FLinearColor::Transparent;
    FLinearColor OptionPressedColor = FLinearColor::Transparent;

    UFUNCTION()
    void HandleDropdownClicked();

    UFUNCTION()
    void HandleDropdownHovered();

    UFUNCTION()
    void HandleDropdownUnhovered();

    UFUNCTION()
    void HandleDropdownPressed();

    UFUNCTION()
    void HandleDropdownReleased();

    UFUNCTION()
    void HandleAnchorOpenChanged(bool bIsOpen);

    UFUNCTION()
    UUserWidget* HandleGetMenuContent();
};
