#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/AssocRowWidget.h"
#include "AssocTabWidget.generated.h"

class ATempUIActor;
class UVerticalBox;
class UTextBlock;

UCLASS()
class DT4MOB_API UAssocTabWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "AssocTab")
    void SetBoundActor(ATempUIActor* Actor);

protected:
    /** List populated with one AssocRowWidget per associated actor. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* AssocList;

    /** Shows "ASSOCIATED (N)". */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* CountLabel;

    /** Blueprint class to instantiate for each row. Set in WBP_AssocTabWidget defaults. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssocTab")
    TSubclassOf<UAssocRowWidget> RowClass;

private:
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    void RebuildList();

    UFUNCTION()
    void HandleOpenRequested(const FString& ThingId);

    UFUNCTION()
    void HandleEntityRegistered(ATempUIActor* Actor);

    UFUNCTION()
    void HandleEntityUnregistered(const FString& ThingId);
};
