#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "JsonTabWidget.generated.h"

class ATempUIActor;
class UJsonViewerWidget;
class UBorder;

UCLASS()
class DT4MOB_API UJsonTabWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "JsonTab")
    void SetBoundActor(ATempUIActor* Actor);

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UJsonViewerWidget* JsonTextBox;

    /** Background panel behind the JSON text, for contrast. Must be named "Border" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Border;

    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    void Refresh();

    UFUNCTION()
    void HandleEntityDataChanged();
};
