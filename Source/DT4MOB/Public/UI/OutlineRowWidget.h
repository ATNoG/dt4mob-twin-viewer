#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "OutlineRowWidget.generated.h"

class UTextBlock;
class UBorder;
class UButton;
class UImage;
class ATempUIActor;
struct FButtonStyle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOutlineRowSelected, const FString&, ThingId);

UCLASS()
class DT4MOB_API UOutlineRowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    void SetData(const FString& InThingId, const FString& InTypeKey, const FString& InDisplayName, ATempUIActor* InActor);
    void SetEvenRow(bool bEven);

    const FString& GetThingId() const { return ThingId; }

    UPROPERTY(BlueprintAssignable)
    FOnOutlineRowSelected OnRowSelected;

    /** @param WorldContextObject Used to resolve UDT4MOBEntityFactory's extension registry. */
    static FLinearColor GetBadgeColor(const UObject* WorldContextObject, const FString& Key);
    static FString GetBadgeLabel(const UObject* WorldContextObject, const FString& Key);

    /** @brief Shared dark rounded-pill button chrome used across row widgets (e.g. "Open ↗", ON/OFF toggles). */
    static FButtonStyle MakePillButtonStyle();

    /**
     * @brief Sets a UBorder's fill color while also matching its outline stroke to the same color.
     *
     * UBorder::SetBrushColor() only touches the brush's TintColor — if the Designer-authored brush
     * bakes in an OutlineSettings.Color (as row backgrounds across the app do), that stroke keeps
     * showing through at whatever color the Blueprint set it to, regardless of theme/striping.
     * Use this instead of SetBrushColor() for any row background that needs to react to theme
     * changes or zebra striping.
     */
    static void SetBorderColorPreservingOutline(UBorder* Border, const FLinearColor& Color);

protected:
    /** Row background/striping. Must be named "Background" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Background;

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

    /** Called when visibility is toggled — swaps the eye icon. */
    UFUNCTION(BlueprintNativeEvent)
    void OnRowVisibilityChanged(bool bVisible);
    virtual void OnRowVisibilityChanged_Implementation(bool bVisible);

    /** Shown eye icon (visible state). Must be named "VisibilityIcon" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* VisibilityIcon;

    /** Shown eye icon (hidden state). Must be named "VisibilityIcon_Hidden" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* VisibilityIcon_Hidden;

    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    FString ThingId;
    FString TypeKey;
    bool bIsVisible = true;
    bool bIsEvenRow = true;
    bool bIsHovered = false;
    FLinearColor EvenRowColor = FLinearColor::Transparent;
    FLinearColor OddRowColor = FLinearColor::Transparent;
    FLinearColor HoverColor = FLinearColor::Transparent;
    FLinearColor TextNormalColor = FLinearColor::White;
    FLinearColor TextHoverColor = FLinearColor::White;

    TWeakObjectPtr<ATempUIActor> BoundActor;

    void RefreshRowBackground();

    UFUNCTION()
    void HandleRowClicked();

    UFUNCTION()
    void HandleVisibilityClicked();

    UFUNCTION()
    void HandleRowHovered();

    UFUNCTION()
    void HandleRowUnhovered();
};
