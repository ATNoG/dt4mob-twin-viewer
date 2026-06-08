#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "OutlineRowWidget.generated.h"

class UTextBlock;
class UBorder;
class UButton;
class ATempUIActor;

UCLASS()
class DT4MOB_API UOutlineRowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    void SetData(const FString& InThingId, const FString& InTypeKey, const FString& InDisplayName, ATempUIActor* InActor);

    const FString& GetThingId() const { return ThingId; }

    static FLinearColor GetBadgeColor(const FString& Key);
    static FString GetBadgeLabel(const FString& Key);

protected:
    /** Badge container — background color tinted by entity type. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UBorder* TypeBadge;

    /** Short uppercase label inside the badge, e.g. "TOLL", "VEHICLE". */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TypeLabel;

    /** Full Ditto thingId, e.g. "car:vehicle-001". */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* EntityIdLabel;

    /** Optional — eye icon button to toggle actor visibility. Wire up in Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* VisibilityButton;

    /** Called when visibility is toggled — Blueprint updates the eye icon here. */
    UFUNCTION(BlueprintNativeEvent)
    void OnRowVisibilityChanged(bool bVisible);
    virtual void OnRowVisibilityChanged_Implementation(bool bVisible) {}

private:
    FString ThingId;
    FString TypeKey;
    bool bIsVisible = true;

    TWeakObjectPtr<ATempUIActor> BoundActor;

    UFUNCTION()
    void HandleVisibilityClicked();
};
