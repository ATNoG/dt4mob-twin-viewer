#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/ModelsRowWidget.h"
#include "ModelsTabWidget.generated.h"

class ATempUIActor;
class UVerticalBox;
class UTextBlock;

UCLASS()
class DT4MOB_API UModelsTabWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "ModelsTab")
    void SetBoundActor(ATempUIActor* Actor);

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* LayerList;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* SectionLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ModelsTab")
    TSubclassOf<UModelsRowWidget> RowClass;

private:
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    void RebuildList();

    UFUNCTION()
    void HandleMeshLayersChanged();
};
