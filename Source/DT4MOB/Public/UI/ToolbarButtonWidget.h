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
 */
UCLASS()
class DT4MOB_API UToolbarButtonWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    /** Root clickable button. Must be named "Button" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* Button;

    /** Optional icon displayed left of the label. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* Icon;

    /** Optional label displayed next to the icon. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* Label;

    /** Background pill behind the button. Must be named "ButtonBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* ButtonBackground;

    UFUNCTION(BlueprintCallable)
    void SetLabel(const FText& Text);

    UFUNCTION(BlueprintCallable)
    void SetIcon(UTexture2D* Texture);

    /**
     * @brief Switches the button between its normal and active visual states.
     * Applies the theme's Accent color as a persistent tint when active.
     */
    UFUNCTION(BlueprintCallable)
    void SetActiveState(bool bActive);

protected:
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    bool bIsActive = false;

    FLinearColor NormalColor = FLinearColor::Transparent;
    FLinearColor HoverColor = FLinearColor::Transparent;
    FLinearColor PressedColor = FLinearColor::Transparent;
    FLinearColor ActiveColor = FLinearColor::Transparent;

    void RefreshButtonStyle();
    void SetBackgroundColor(const FLinearColor& Color);

    UFUNCTION()
    void HandleButtonHovered();

    UFUNCTION()
    void HandleButtonUnhovered();

    UFUNCTION()
    void HandleButtonPressed();

    UFUNCTION()
    void HandleButtonReleased();
};
