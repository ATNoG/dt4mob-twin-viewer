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
 * OnGetUserMenuContentEvent in C++ — see OptionsAppearanceSectionWidget::HandleGetMenuContent.
 * MenuAnchor requires popup content to be a UUserWidget, so this can't just be the plain
 * Border it used to be; SetOwningAnchor() gives it a way to close itself after a pick.
 */
UCLASS()
class DT4MOB_API UThemeDropdownPopupWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    /** Call right after CreateWidget, before returning this from OnGetUserMenuContentEvent. */
    void SetOwningAnchor(UMenuAnchor* InAnchor);

protected:
    /** Popup panel background. Must be named "PopupBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* PopupBackground;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* DarkOptionButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* DarkOptionLabel;

    /** Background pill behind DarkOptionButton. Must be named "DarkOptionBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* DarkOptionBackground;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* LightOptionButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* LightOptionLabel;

    /** Background pill behind LightOptionButton. Must be named "LightOptionBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* LightOptionBackground;

    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    TWeakObjectPtr<UMenuAnchor> OwningAnchor;

    FLinearColor OptionNormalColor = FLinearColor::Transparent;
    FLinearColor OptionHoverColor = FLinearColor::Transparent;
    FLinearColor OptionPressedColor = FLinearColor::Transparent;

    void SelectDarkMode(bool bDarkMode);
    void SetOptionBackgroundColor(UBorder* Border, const FLinearColor& Color);

    UFUNCTION()
    void HandleDarkOptionClicked();

    UFUNCTION()
    void HandleLightOptionClicked();

    UFUNCTION()
    void HandleDarkOptionHovered();

    UFUNCTION()
    void HandleDarkOptionUnhovered();

    UFUNCTION()
    void HandleDarkOptionPressed();

    UFUNCTION()
    void HandleDarkOptionReleased();

    UFUNCTION()
    void HandleLightOptionHovered();

    UFUNCTION()
    void HandleLightOptionUnhovered();

    UFUNCTION()
    void HandleLightOptionPressed();

    UFUNCTION()
    void HandleLightOptionReleased();
};
