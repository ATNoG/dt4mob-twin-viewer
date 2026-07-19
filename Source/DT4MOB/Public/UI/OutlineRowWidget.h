#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "OutlineRowWidget.generated.h"

class UTextBlock;
class UBorder;
class UButton;
class ATempUIActor;
struct FButtonStyle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOutlineRowSelected, const FString&, ThingId);

UCLASS()
class DT4MOB_API UOutlineRowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    void SetData(const FString& InThingId, const FString& InTypeKey, const FString& InDisplayName, ATempUIActor* InActor);
    void SetEvenRow(bool bEven);

    const FString& GetThingId() const { return ThingId; }

    UPROPERTY(BlueprintAssignable)
    FOnOutlineRowSelected OnRowSelected;

    static FLinearColor GetBadgeColor(const FString& Key);
    static FString GetBadgeLabel(const FString& Key);

    /** @brief Shared dark rounded-pill button chrome used across row widgets (e.g. "Open ↗", ON/OFF toggles). */
    static FButtonStyle MakePillButtonStyle();

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* RowBackground;

    /** Badge container — background color tinted by entity type. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UBorder* TypeBadge;

    /** Short uppercase label inside the badge, e.g. "TOLL", "VEHICLE". */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TypeLabel;

    /** Full Ditto thingId, e.g. "car:vehicle-001". */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* EntityIdLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* RowButton;

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
    void HandleRowClicked();

    UFUNCTION()
    void HandleVisibilityClicked();
};
