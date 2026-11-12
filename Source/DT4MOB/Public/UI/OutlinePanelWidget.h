/**
 * @file
 * @brief Declares UOutlinePanelWidget, the slide-out list of all loaded entities.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/OutlineRowWidget.h"
#include "OutlinePanelWidget.generated.h"

class ATempUIActor;
class UActorRegistryService;
class UButton;
class UEditableText;
class UScrollBox;
class UBorder;
class UWidgetAnimation;
class UTextBlock;

/**
 * @brief Fires when the user clicks an entity in the outline panel.
 * @param Actor  Registered actor for the clicked row.
 * @ingroup ui
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityOpenRequested, ATempUIActor*, Actor);

/**
 * @brief Slide-out "Entities" panel that lists every registered entity actor, with a search filter.
 *
 * The rows mirror UActorRegistryService: a row is added on OnEntityRegistered, removed on
 * OnEntityUnregistered, and the whole list is rebuilt each time the panel opens. The search box
 * hides rows whose thingId does not contain the typed text (case-insensitive). Clicking a row
 * broadcasts OnEntityOpenRequested, which URootHUDWidget uses to open an entity window.
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UOutlinePanelWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Binds the close button, the search box and the finished event of CloseAnimation.
     * @return False if the base UUserWidget::Initialize() fails, true otherwise.
     */
    virtual bool Initialize() override;

    /** @brief Subscribes to UActorRegistryService's register and unregister events the first time the widget is constructed. */
    virtual void NativeConstruct() override;

    /**
     * @brief Opens or closes the panel.
     *
     * Opening makes the panel visible (SelfHitTestInvisible), rebuilds the list and plays
     * OpenAnimation. Closing plays CloseAnimation and collapses the panel when it finishes, or
     * collapses it at once if there is no CloseAnimation.
     */
    void TogglePanel();

    /** @brief Broadcast when a row is clicked and its thing still has a registered actor. URootHUDWidget opens or focuses an entity window for it. */
    UPROPERTY(BlueprintAssignable)
    FOnEntityOpenRequested OnEntityOpenRequested;

    /**
     * @brief Whether the panel is open.
     * @return True while open. Turns false as soon as closing starts, before the close animation ends.
     */
    bool IsOpen() const { return bIsOpen; }

protected:
    /** @brief Panel header ("Entities"), in the theme's TextPrimary. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* HeaderLabel;

    /** @brief Closes the panel. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* CloseButton;

    /** @brief Search field; rows whose thingId does not contain its text (case-insensitive) are collapsed. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UEditableText* SearchBox;

    /** @brief Scroll box that holds the rows. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UScrollBox* EntityListBox;

    /** @brief Header background, tinted with the theme's PanelBackground. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* HeaderBorder;

    /** @brief Search field background (theme SearchBackground, with a WindowOutline stroke). Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* SearchBorder;

    /** @brief List background, tinted with the theme's ListBackground. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* ListBorder;

    /** @brief "All Entities" list heading, in the theme's TextSecondary. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* OverviewText;

    /**
     * @brief Colors the header, search field, list background, heading and search text from Theme.
     * @param Theme  Active theme; nothing changes if it is null.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

    /** @brief Widget class created for each row; set it to the WBP_OutlineRow asset in the Blueprint defaults. If unset, no rows are created. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline")
    TSubclassOf<UOutlineRowWidget> RowWidgetClass;

    /** @brief Slide-in animation played when the panel opens. Optional; must be named "OpenAnimation" in the Blueprint's Animations panel. */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    UWidgetAnimation* OpenAnimation;

    /** @brief Slide-out animation played when the panel closes. Optional; must be named "CloseAnimation" in the Blueprint's Animations panel. */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    UWidgetAnimation* CloseAnimation;

private:
    /** @brief Open state toggled by TogglePanel(). */
    bool bIsOpen = false;

    /** @brief Source of the listed actors. Resolved in the first NativeConstruct(). */
    UPROPERTY()
    UActorRegistryService* Registry = nullptr;

    /** @brief Rows currently in EntityListBox, in the order they were added. Not a UPROPERTY; EntityListBox keeps the rows alive. */
    TArray<UOutlineRowWidget*> AllRows;

    /** @brief Clears the list, adds a row for every actor in the registry and re-applies the search filter. */
    void PopulateAll();
    /**
     * @brief Creates a row for Actor, labels it with the type key from UDT4MOBEntityFactory,
     *        stripes it and hides it if it does not match the current search text.
     * @param Actor  Actor to add; ignored if not valid.
     */
    void AddRow(ATempUIActor* Actor);
    /** @brief Plays OpenAnimation, if bound. */
    void PlayOpenAnimation();
    /** @brief Plays CloseAnimation, or collapses the panel at once if it is not bound. */
    void PlayCloseAnimation();

    /** @brief Collapses the panel once CloseAnimation has finished. */
    UFUNCTION()
    void HandleCloseAnimationFinished();

    /** @brief Closes the panel if it is open. */
    UFUNCTION()
    void HandleCloseClicked();

    /**
     * @brief Shows the rows whose thingId contains Text (case-insensitive) and collapses the rest.
     * @param Text  Current search text; empty shows every row.
     */
    UFUNCTION()
    void HandleSearchChanged(const FText& Text);

    /**
     * @brief Adds a row for an actor that was just registered.
     * @param Actor  Newly registered actor.
     */
    UFUNCTION()
    void HandleEntityRegistered(ATempUIActor* Actor);

    /**
     * @brief Removes the row for a thing that was unregistered. The striping of the remaining rows is not recomputed.
     * @param ThingId  Ditto thingId of the removed actor.
     */
    UFUNCTION()
    void HandleEntityUnregistered(const FString& ThingId);

    /**
     * @brief Looks up the actor for ThingId in the registry and broadcasts OnEntityOpenRequested if it exists.
     * @param ThingId  Ditto thingId of the clicked row.
     */
    UFUNCTION()
    void HandleRowSelected(const FString& ThingId);
};
