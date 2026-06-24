#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/InfoFieldTypes.h"
#include "InfoTabWidget.generated.h"

class ATempUIActor;
class UVerticalBox;
class UTextBlock;
class UButton;
class UInfoFieldRegistry;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInfoConfigureRequested);

UCLASS()
class DT4MOB_API UInfoTabWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "InfoTab")
    void SetBoundActor(ATempUIActor* Actor);

    /** Fired when the user clicks the configure button — EntityWindowWidget handles the panel. */
    UPROPERTY(BlueprintAssignable, Category = "InfoTab")
    FOnInfoConfigureRequested OnConfigureRequested;

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* PropertyList;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* ConfigureBtn;

    /** Font for the row label (left side). Set in Blueprint defaults. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InfoTab")
    FSlateFontInfo LabelFont;

    /** Font for the row value (right side). Set in Blueprint defaults. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InfoTab")
    FSlateFontInfo ValueFont;

    /** Color for row labels. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InfoTab")
    FLinearColor LabelColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.f);  // ~#7a7a7a

    /** Color for row values. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InfoTab")
    FLinearColor ValueColor = FLinearColor(0.72f, 0.72f, 0.72f, 1.f); // ~#dcdcdc

private:
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    FString CachedTypeKey;

    UPROPERTY()
    TArray<UTextBlock*> LabelBlocks;

    UPROPERTY()
    TArray<UTextBlock*> ValueBlocks;

    /** Call from Blueprint's ApplyTheme after updating LabelColor/ValueColor. */
    UFUNCTION(BlueprintCallable, Category = "InfoTab")
    void RefreshColors();

    void RebuildRows();
    void RefreshValues();

    UFUNCTION()
    void HandleEntityDataChanged();

    UFUNCTION()
    void HandleFieldsChanged(const FString& TypeKey);

    UFUNCTION()
    void HandleConfigureClicked();

    UInfoFieldRegistry* GetRegistry() const;
};
