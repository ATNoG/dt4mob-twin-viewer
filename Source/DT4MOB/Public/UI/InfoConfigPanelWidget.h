#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/InfoFieldTypes.h"
#include "InfoConfigPanelWidget.generated.h"

class ATempUIActor;
class UInfoFieldRegistry;
class UScrollBox;
class UButton;
class UBorder;
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
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UScrollBox* CandidateList;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* SaveBtn;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* SaveBtnBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* ResetBtn;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* ResetBtnBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UCheckBox* SelectAllCB;

private:
    FString CachedTypeKey;

    UPROPERTY()
    UInfoFieldRegistry* CachedRegistry = nullptr;

    TArray<FInfoFieldCandidate> Candidates;

    UPROPERTY()
    TArray<UCheckBox*> CheckBoxes;

    bool bUpdatingSelectAll = false;

    void BuildRows();
    void UpdateSelectAllState();

    UFUNCTION() void HandleSave();
    UFUNCTION() void HandleSaveBtnHovered();
    UFUNCTION() void HandleSaveBtnUnhovered();
    UFUNCTION() void HandleSaveBtnPressed();
    UFUNCTION() void HandleSaveBtnReleased();

    UFUNCTION() void HandleReset();
    UFUNCTION() void HandleResetBtnHovered();
    UFUNCTION() void HandleResetBtnUnhovered();
    UFUNCTION() void HandleResetBtnPressed();
    UFUNCTION() void HandleResetBtnReleased();

    UFUNCTION() void HandleSelectAllChanged(bool bIsChecked);
    UFUNCTION() void HandleRowCheckChanged(bool bIsChecked);

    FLinearColor SaveBorderNormal;
    FLinearColor SaveBorderHovered;
    FLinearColor SaveBorderPressed;

    FLinearColor ResetBorderNormal;
    FLinearColor ResetBorderHovered;
    FLinearColor ResetBorderPressed;
};
