/**
 * @file
 * @brief Declares UOutlineRowWidget, one entity row in the outline panel, plus row styling helpers shared by other list widgets.
 * @ingroup ui
 */
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

/**
 * @brief Fires when an outline row is clicked.
 * @param ThingId  Ditto thingId of the clicked row.
 * @ingroup ui
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOutlineRowSelected, const FString&, ThingId);

/**
 * @brief One row of UOutlinePanelWidget: a colored type badge, the entity's thingId and an
 *        optional eye button that hides or shows the actor.
 *
 * Also hosts two static helpers that other list rows use: MakePillButtonStyle() and
 * SetBorderColorPreservingOutline().
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UOutlineRowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Binds the click and hover events of RowButton and the click event of VisibilityButton.
     * @return False if the base UUserWidget::Initialize() fails, true otherwise.
     */
    virtual bool Initialize() override;

    /**
     * @brief Fills the row for one entity and styles its type badge.
     *
     * Also resets the row to the visible state and updates the eye icons, without checking
     * whether the actor is currently hidden.
     *
     * @param InThingId      Ditto thingId, shown in EntityIdLabel.
     * @param InTypeKey      Entity type key; selects the badge label and color.
     * @param InDisplayName  Display name of the type. Currently not used by the row.
     * @param InActor        Actor that the eye button hides and shows.
     */
    void SetData(const FString& InThingId, const FString& InTypeKey, const FString& InDisplayName, ATempUIActor* InActor);

    /**
     * @brief Sets the zebra striping state. Even rows use the theme's RowBackgroundEven, odd rows are transparent.
     * @param bEven  True for an even row.
     */
    void SetEvenRow(bool bEven);

    /**
     * @brief Ditto thingId of this row.
     * @return The thingId passed to SetData(), or an empty string before that.
     */
    const FString& GetThingId() const { return ThingId; }

    /** @brief Broadcast with the row's thingId when RowButton is clicked. */
    UPROPERTY(BlueprintAssignable)
    FOnOutlineRowSelected OnRowSelected;

    /**
     * @brief Badge color for an entity type, taken from its UEntityTypeExtension.
     * @param WorldContextObject  Used to reach the GameInstance and its UDT4MOBEntityFactory.
     * @param Key                 Entity type key.
     * @return The extension's badge color, or neutral gray if the factory is not available.
     */
    static FLinearColor GetBadgeColor(const UObject* WorldContextObject, const FString& Key);

    /**
     * @brief Short uppercase badge label for an entity type, taken from its UEntityTypeExtension.
     * @param WorldContextObject  Used to reach the GameInstance and its UDT4MOBEntityFactory.
     * @param Key                 Entity type key.
     * @return The extension's label, or the first six characters of Key in upper case if the factory is not available.
     */
    static FString GetBadgeLabel(const UObject* WorldContextObject, const FString& Key);

    /**
     * @brief Builds the dark rounded pill button style shared by row widgets (for example the
     *        "Open" button of association rows and the ON/OFF toggles of the Models tab).
     * @return The engine's default button style with fixed dark normal, hovered and pressed brushes and padding.
     */
    static FButtonStyle MakePillButtonStyle();

    /**
     * @brief Sets a UBorder's fill color and makes its outline stroke the same color.
     *
     * UBorder::SetBrushColor() only changes the brush's TintColor. If the Designer brush has its
     * own OutlineSettings.Color (as the row backgrounds in this app do), that stroke keeps the
     * Blueprint color whatever the theme or striping. Use this instead of SetBrushColor() for any
     * row background that reacts to theme changes or striping. A brush left at NoDrawType is
     * switched to Box so the color becomes visible.
     *
     * @param Border  Border to recolor; ignored if null.
     * @param Color   New fill and outline color.
     */
    static void SetBorderColorPreservingOutline(UBorder* Border, const FLinearColor& Color);

protected:
    /** @brief Row background used for striping and the hover highlight. Optional; must be named "Background" in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Background;

    /** @brief Badge container, filled with the type color at 15% opacity and outlined at 40%. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UBorder* TypeBadge;

    /** @brief Short uppercase type label inside the badge (for example "TOLL"), drawn in the type color. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TypeLabel;

    /** @brief Full Ditto thingId, for example "car:vehicle-001". TextSecondary normally, TextPrimary while hovered. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* EntityIdLabel;

    /** @brief Clickable area of the row; a click broadcasts OnRowSelected and hovering highlights the row. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* RowButton;

    /** @brief Eye button that hides or shows the actor in game. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* VisibilityButton;

    /**
     * @brief Updates the eye icons after the row's visibility state changes. Can be overridden in Blueprint.
     * @param bVisible  True if the actor is now visible.
     */
    UFUNCTION(BlueprintNativeEvent)
    void OnRowVisibilityChanged(bool bVisible);
    /**
     * @brief Default implementation: shows VisibilityIcon when visible and VisibilityIcon_Hidden when hidden.
     * @param bVisible  True if the actor is now visible.
     */
    virtual void OnRowVisibilityChanged_Implementation(bool bVisible);

    /** @brief Eye icon shown while the actor is visible. Optional; must be named "VisibilityIcon" in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* VisibilityIcon;

    /** @brief Icon shown while the actor is hidden. Optional; must be named "VisibilityIcon_Hidden" in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* VisibilityIcon_Hidden;

    /**
     * @brief Takes the row, hover and text colors from Theme and repaints the row.
     * @param Theme  Active theme; nothing changes if it is null.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    /** @brief Ditto thingId set by SetData(). */
    FString ThingId;
    /** @brief Entity type key set by SetData(). */
    FString TypeKey;
    /** @brief Visibility state the eye button last applied to the actor. */
    bool bIsVisible = true;
    /** @brief Striping state set by SetEvenRow(). */
    bool bIsEvenRow = true;
    /** @brief True while the cursor is over RowButton. */
    bool bIsHovered = false;
    /** @brief Background of even rows (theme RowBackgroundEven). */
    FLinearColor EvenRowColor = FLinearColor::Transparent;
    /** @brief Background of odd rows (always transparent). */
    FLinearColor OddRowColor = FLinearColor::Transparent;
    /** @brief Background while hovered (theme RowHover). */
    FLinearColor HoverColor = FLinearColor::Transparent;
    /** @brief thingId text color at rest (theme TextSecondary). */
    FLinearColor TextNormalColor = FLinearColor::White;
    /** @brief thingId text color while hovered (theme TextPrimary). */
    FLinearColor TextHoverColor = FLinearColor::White;

    /** @brief Actor that the eye button hides and shows. Weak, so a destroyed actor is simply ignored. */
    TWeakObjectPtr<ATempUIActor> BoundActor;

    /** @brief Applies the hover, even or odd background and the matching thingId text color. */
    void RefreshRowBackground();

    /** @brief Broadcasts OnRowSelected with ThingId. */
    UFUNCTION()
    void HandleRowClicked();

    /** @brief Flips the actor's hidden-in-game state and updates the eye icons. Does nothing if the actor is gone. */
    UFUNCTION()
    void HandleVisibilityClicked();

    /** @brief Switches the row to its hover colors. */
    UFUNCTION()
    void HandleRowHovered();

    /** @brief Restores the row's striped colors. */
    UFUNCTION()
    void HandleRowUnhovered();
};
