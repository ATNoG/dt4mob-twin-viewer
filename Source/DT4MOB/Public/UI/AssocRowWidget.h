#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "AssocRowWidget.generated.h"

class ATempUIActor;
class UTextBlock;
class UButton;
class UBorder;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssocRowOpenRequested, const FString&, ThingId);

UCLASS()
class DT4MOB_API UAssocRowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    void SetActor(ATempUIActor* Actor);
    void SetEvenRow(bool bEven);

    UPROPERTY(BlueprintAssignable)
    FOnAssocRowOpenRequested OnOpenRequested;

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* RowBackground;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TypeBadge;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TypeLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ThingIdLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* OpenButton;

private:
    FString CachedThingId;

    UFUNCTION()
    void HandleOpenClicked();
};
