/**
 * @file
 * @brief Declares UAssocRowWidget, one row in the entity window's Associated tab.
 * @ingroup ui
 */

#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "AssocRowWidget.generated.h"

class ATempUIActor;
class UTextBlock;
class UButton;
class UBorder;

/**
 * @brief Fired when the user clicks the open button of an Associated tab row.
 * @param ThingId  thingId of the associated entity shown in the row.
 * @ingroup ui
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssocRowOpenRequested, const FString&, ThingId);

/**
 * @brief Row in the Associated tab that shows one associated entity: a type badge, its thingId and an open button.
 *
 * UAssocTabWidget creates one row per associated entity, calls SetActor() and SetEvenRow(),
 * and listens to OnOpenRequested to select that entity.
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UAssocRowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Binds the open button's click handler and gives the button the shared pill style.
     * @return False if the base class fails to initialize.
     */
    virtual bool Initialize() override;

    /**
     * @brief Shows the actor's thingId and its type badge.
     *
     * The badge label and color come from the type key that UDT4MOBEntityFactory resolves for
     * the thingId. The row keeps only the thingId, not the actor pointer.
     *
     * @param Actor  Associated entity to display. Ignored if not valid.
     */
    void SetActor(ATempUIActor* Actor);

    /**
     * @brief Sets the zebra stripe background of the row.
     * @param bEven  True for even rows (theme RowBackgroundEven), false for odd rows (transparent).
     */
    void SetEvenRow(bool bEven);

    /** @brief Fired with the row's thingId when the open button is clicked. */
    UPROPERTY(BlueprintAssignable)
    FOnAssocRowOpenRequested OnOpenRequested;

protected:
    /**
     * @brief Stores the theme's even row color and repaints the row background.
     * @param Theme  Active theme. Ignored if null.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

    /** @brief Row background that carries the zebra stripe color. Optional; set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* RowBackground;

    /** @brief Rounded border behind TypeLabel, tinted with the type's badge color. Optional; set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TypeBadge;

    /** @brief Short type label drawn in the badge color. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TypeLabel;

    /** @brief Shows the associated entity's thingId. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ThingIdLabel;

    /** @brief Button that broadcasts OnOpenRequested. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* OpenButton;

private:
    /** @brief thingId shown in the row and sent with OnOpenRequested. */
    FString CachedThingId;

    /** @brief Whether the row uses the even stripe color. Kept so a theme change can repaint it. */
    bool bIsEvenRow = true;

    /** @brief Background color of even rows, taken from the theme. */
    FLinearColor EvenRowColor = FLinearColor::Transparent;

    /** @brief Background color of odd rows. Always transparent. */
    FLinearColor OddRowColor = FLinearColor::Transparent;

    /** @brief Broadcasts OnOpenRequested with CachedThingId. */
    UFUNCTION()
    void HandleOpenClicked();
};
