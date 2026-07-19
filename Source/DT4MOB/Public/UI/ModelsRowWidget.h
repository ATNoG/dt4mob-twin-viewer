#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "ModelsRowWidget.generated.h"

class ATempUIActor;
class UTextBlock;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLayerVisibilityToggled, const FString&, LayerName, bool, bVisible);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLayerTransparencyToggled, const FString&, LayerName, bool, bTranslucent);

UCLASS()
class DT4MOB_API UModelsRowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    void SetEntry(ATempUIActor* Actor, const FString& LayerName);

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

private:
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    FString CachedLayerName;

    void RefreshToggleLabel();
    void RefreshTransparencyLabel();

    UFUNCTION()
    void HandleToggleClicked();

    UFUNCTION()
    void HandleTransparencyClicked();
};
