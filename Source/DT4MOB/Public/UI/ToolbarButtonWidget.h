/**
 * @file
 * @brief Declares UToolbarButtonWidget, a themed toolbar button with icon and label.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "UI/ThemedWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "ToolbarButtonWidget.generated.h"

/**
 * @brief C++ base for all reusable toolbar buttons.
 *
 * Blueprint children (e.g. WBP_ToolbarButton) provide the visual layout.
 * ToolbarWidget binds to UToolbarButtonWidget* instances and wires click
 * events via Button->OnClicked directly.
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UToolbarButtonWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Binds the hover and press handlers, hides the engine button brushes, and collapses an empty label.
     * @return False if the base widget failed to initialise.
     */
    virtual bool Initialize() override;

    /** @brief Root clickable button. Must be named "Button" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* Button;

    /** @brief Optional icon displayed left of the label. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* Icon;

    /** @brief Optional label displayed next to the icon. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* Label;

    /** @brief Background pill behind the button. Must be named "ButtonBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* ButtonBackground;

    /**
     * @brief Sets the label text; an empty text collapses the label.
     * @param Text New label.
     */
    UFUNCTION(BlueprintCallable)
    void SetLabel(const FText& Text);

    /**
     * @brief Sets the icon image. Does nothing if there is no Icon widget or Texture is null.
     * @param Texture Icon texture.
     */
    UFUNCTION(BlueprintCallable)
    void SetIcon(UTexture2D* Texture);

    /**
     * @brief Switches the button between its normal and active visual states.
     * Applies the theme's Accent color as a persistent tint when active.
     * @param bActive True for the active state.
     */
    UFUNCTION(BlueprintCallable)
    void SetActiveState(bool bActive);

protected:
    /**
     * @brief Reads the normal, hover, pressed and active colours from the theme and repaints.
     * @param Theme Active theme.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    /** @brief True while the button shows its active state. */
    bool bIsActive = false;

    FLinearColor NormalColor = FLinearColor::Transparent;  ///< Background at rest, from the theme.
    FLinearColor HoverColor = FLinearColor::Transparent;   ///< Background on hover, from the theme.
    FLinearColor PressedColor = FLinearColor::Transparent; ///< Background while pressed, from the theme.
    FLinearColor ActiveColor = FLinearColor::Transparent;  ///< Background in the active state, from the theme.

    /** @brief Paints the background with ActiveColor or NormalColor depending on bIsActive. */
    void RefreshButtonStyle();

    /**
     * @brief Tints the background brush and its outline, keeping the brush shape from the Designer.
     * @param Color New colour.
     */
    void SetBackgroundColor(const FLinearColor& Color);

    UFUNCTION()
    void HandleButtonHovered();   ///< Paints the hover colour.

    UFUNCTION()
    void HandleButtonUnhovered(); ///< Restores the normal or active colour.

    UFUNCTION()
    void HandleButtonPressed();   ///< Paints the pressed colour.

    UFUNCTION()
    void HandleButtonReleased();  ///< Paints the hover colour.
};
