#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "ModelsRowWidget.generated.h"

class ATempUIActor;
class UTextBlock;
class UButton;
class UBorder;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLayerVisibilityToggled, const FString&, LayerName, bool, bVisible);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLayerTransparencyToggled, const FString&, LayerName, bool, bTranslucent);

UCLASS()
class DT4MOB_API UModelsRowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    void SetEntry(ATempUIActor* Actor, const FString& LayerName);

    /** @brief Alternates row background between the theme's even-row color and transparent, matching
     *  the zebra striping used by outline/assoc list rows elsewhere in the app. */
    void SetEvenRow(bool bEven);

    /** @brief Visually nests this row under a parent group: shifts the label right and shrinks its
     *  font by IndentPx/FontShrinkPerLevel per level. Level 0 (default) leaves the row untouched. */
    void SetIndentLevel(int32 Level);

    UPROPERTY(BlueprintAssignable)
    FOnLayerVisibilityToggled OnVisibilityToggled;

    UPROPERTY(BlueprintAssignable)
    FOnLayerTransparencyToggled OnTransparencyToggled;

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* LayerNameLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* ToggleButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ToggleLabel;

    /** @brief Optional — toggles the layer between its original material and the configured ghost material. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* TransparencyButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* TransparencyLabel;

    /** Row background. Must be named "Border" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Border;

    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    FString CachedLayerName;

    bool bIsEvenRow = true;
    FLinearColor EvenRowColor = FLinearColor::Transparent;
    FLinearColor OddRowColor = FLinearColor::Transparent;

    /** @brief Horizontal shift (px) applied to LayerNameLabel per indent level. */
    static constexpr float IndentPxPerLevel = 16.f;

    /** @brief Font size reduction (pt) applied to LayerNameLabel per indent level. */
    static constexpr int32 FontShrinkPerLevel = 1;

    void RefreshRowBackground();
    void RefreshToggleLabel();
    void RefreshTransparencyLabel();

    UFUNCTION()
    void HandleToggleClicked();

    UFUNCTION()
    void HandleTransparencyClicked();
};
