#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "UI/ThemedWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
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
    /** Root clickable button. Must be named "Button" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* Button;

    /** Optional icon displayed left of the label. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* Icon;

    /** Optional label displayed next to the icon. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* Label;

    /**
     * @brief Switches the button between its normal and active visual states.
     * Blueprint implements the colour/style change (e.g. orange tint when active).
     */
    UFUNCTION(BlueprintCallable)
    void SetLabel(const FText& Text);

    UFUNCTION(BlueprintCallable)
    void SetIcon(UTexture2D* Texture);

    UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
    void SetActiveState(bool bActive);
};
