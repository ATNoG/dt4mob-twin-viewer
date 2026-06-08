#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "ModelsRowWidget.generated.h"

class ATempUIActor;
class UTextBlock;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLayerVisibilityToggled, const FString&, LayerName, bool, bVisible);

UCLASS()
class DT4MOB_API UModelsRowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    void SetEntry(ATempUIActor* Actor, const FString& LayerName);

    UPROPERTY(BlueprintAssignable)
    FOnLayerVisibilityToggled OnVisibilityToggled;

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* LayerNameLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* ToggleButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ToggleLabel;

private:
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    FString CachedLayerName;

    void RefreshToggleLabel();

    UFUNCTION()
    void HandleToggleClicked();
};
