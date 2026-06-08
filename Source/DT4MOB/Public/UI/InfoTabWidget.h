#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/InfoFieldTypes.h"
#include "InfoTabWidget.generated.h"

class ATempUIActor;
class UVerticalBox;
class UTextBlock;
class UInfoFieldRegistry;

UCLASS()
class DT4MOB_API UInfoTabWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "InfoTab")
    void SetBoundActor(ATempUIActor* Actor);

protected:
    /** Container populated with one row per info field. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* PropertyList;

    /** Font for the row label (left side). Set in Blueprint defaults. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InfoTab")
    FSlateFontInfo LabelFont;

    /** Font for the row value (right side). Set in Blueprint defaults. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InfoTab")
    FSlateFontInfo ValueFont;

    /** Color for row labels. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InfoTab")
    FLinearColor LabelColor = FLinearColor(0.48f, 0.48f, 0.48f, 1.f);

    /** Color for row values. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InfoTab")
    FLinearColor ValueColor = FLinearColor(1.f, 1.f, 1.f, 1.f);

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

    UInfoFieldRegistry* GetRegistry() const;
};
