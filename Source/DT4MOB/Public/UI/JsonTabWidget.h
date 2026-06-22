#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "JsonTabWidget.generated.h"

class ATempUIActor;
class UJsonViewerWidget;

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

private:
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    void Refresh();

    UFUNCTION()
    void HandleEntityDataChanged();
};
