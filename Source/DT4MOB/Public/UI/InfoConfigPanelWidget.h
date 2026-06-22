#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/InfoFieldTypes.h"
#include "InfoConfigPanelWidget.generated.h"

class ATempUIActor;
class UInfoFieldRegistry;
class UScrollBox;
class UButton;
class UCheckBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConfigPanelClosed);

UCLASS()
class DT4MOB_API UInfoConfigPanelWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    void Setup(ATempUIActor* Actor, const FString& TypeKey, UInfoFieldRegistry* Registry);

    UPROPERTY(BlueprintAssignable)
    FOnConfigPanelClosed OnClosed;

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UScrollBox* CandidateList;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* SaveBtn;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* ResetBtn;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* CloseBtn;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* SelectAllBtn;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* DeselectAllBtn;

private:
    FString CachedTypeKey;

    UPROPERTY()
    UInfoFieldRegistry* CachedRegistry = nullptr;

    // Parallel arrays: one entry per visible candidate row
    TArray<FInfoFieldCandidate> Candidates;

    UPROPERTY()
    TArray<UCheckBox*> CheckBoxes;

    void BuildRows();

    UFUNCTION() void HandleSave();
    UFUNCTION() void HandleReset();
    UFUNCTION() void HandleClose();
    UFUNCTION() void HandleSelectAll();
    UFUNCTION() void HandleDeselectAll();
};
