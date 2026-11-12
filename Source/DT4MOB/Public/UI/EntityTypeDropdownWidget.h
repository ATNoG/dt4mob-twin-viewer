/**
 * @file
 * @brief Declares UEntityTypeDropdownWidget, the entity type picker in the toolbar.
 * @ingroup ui
 */

#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/DropdownOptionWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ScrollBox.h"
#include "EntityTypeDropdownWidget.generated.h"

/**
 * @brief Fired when the user picks an entity type in the dropdown.
 * @param TypeKey  Selected type key. Empty means "None".
 * @ingroup ui
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityTypeSelected, const FString&, TypeKey);

/**
 * @brief Fired when the dropdown popup opens or closes.
 * @param bOpen  True if the popup is now open.
 * @ingroup ui
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDropdownStateChanged, bool, bOpen);

/**
 * @brief Entity type picker in the toolbar.
 *
 * Lists the registered entity types after a leading "None" entry. UToolbarWidget fills it
 * with PopulateTypes() and listens to OnTypeSelected to set the type used for entity
 * placement.
 *
 * C++ owns all state (selected type, open or closed, available types). PopulateTypes,
 * SelectType, ToggleDropdown and CloseDropdown are BlueprintNativeEvents: the C++
 * _Implementation updates the state and the built-in visuals, and a Blueprint override can
 * add its own visual response (animate the popup, highlight rows) and then call the parent
 * to keep the state in sync.
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UEntityTypeDropdownWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /** @brief Fires when the user picks a type. Empty string means None. */
    UPROPERTY(BlueprintAssignable)
    FOnEntityTypeSelected OnTypeSelected;

    /** @brief Fires whenever the dropdown opens or closes. Blueprint can bind here to drive visuals. */
    UPROPERTY(BlueprintAssignable)
    FOnDropdownStateChanged OnDropdownStateChanged;

    /**
     * @brief Replaces the option list with a "None" entry followed by one entry per type key.
     *
     * The C++ implementation stores TypeKeys, clears OptionList and creates an OptionClass
     * widget per entry, labelled with the type's display name from UDT4MOBEntityFactory and
     * wired to SelectType(). Rows are only built when OptionList and OptionClass are set.
     * A Blueprint override must call the parent; the parent clears OptionList, so rows added
     * before that call are lost.
     *
     * @param TypeKeys  Type keys to list, in display order, without the empty "None" key.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void PopulateTypes(const TArray<FString>& TypeKeys);

    /**
     * @brief Binds an option widget's OnOptionClicked to this dropdown so a click calls SelectType().
     *
     * PopulateTypes() already does this for the rows it creates. Call it only for option
     * widgets created elsewhere, and only once per widget.
     *
     * @param Option  Option widget to bind. Ignored if null.
     */
    UFUNCTION(BlueprintCallable)
    void RegisterOption(UDropdownOptionWidget* Option);

    /**
     * @brief Selects a type, closes the popup and broadcasts OnTypeSelected.
     *
     * The header shows "None" for an empty key, otherwise InDisplayName, or the key itself
     * when InDisplayName is empty. A Blueprint override can update the highlighted row and
     * then call the parent.
     *
     * @param TypeKey        Type key to select. Pass an empty string to select None.
     * @param InDisplayName  Header text for the type. Empty falls back to TypeKey.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void SelectType(const FString& TypeKey, const FString& InDisplayName = TEXT(""));

    /**
     * @brief Toggles the popup open or closed and broadcasts OnDropdownStateChanged.
     *
     * Shows or collapses DropdownPopup and keeps the header button hover-tinted while the
     * popup is open. A Blueprint override can animate the popup and then call the parent.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void ToggleDropdown();

    /**
     * @brief Closes the popup without changing the selection.
     *
     * Collapses DropdownPopup and broadcasts OnDropdownStateChanged(false), even if the popup
     * was already closed. A Blueprint override can hide the popup panel and then call the parent.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void CloseDropdown();

    /** @return The selected type key. Empty string means None. */
    UFUNCTION(BlueprintPure)
    FString GetSelectedType() const { return SelectedType; }

    /** @return The type keys from the last PopulateTypes() call, without the None entry. */
    UFUNCTION(BlueprintPure)
    TArray<FString> GetAvailableTypes() const { return AvailableTypes; }

    /** @return True if the popup is currently open. */
    UFUNCTION(BlueprintPure)
    bool IsDropdownOpen() const { return bIsOpen; }

protected:
    /** @brief Clickable header button with the "Type" label, the selected type and the arrow. Must be named "DropdownButton". */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* DropdownButton;

    /** @brief Displays the currently selected type in the header. Must be named "SelectedTypeText". */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* SelectedTypeText;

    /** @brief Header button background. Must be named "ButtonBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* ButtonBackground;

    /** @brief Popup panel background, visible while the dropdown is open. Must be named "DropdownPopup" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* DropdownPopup;

    /** @brief Static "Type" label left of the selected type text. Must be named "Type" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* Type;

    /** @brief Dropdown arrow icon. Must be named "Arrow" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* Arrow;

    /** @brief Option list container. Must be named "OptionList" in the Blueprint layout; without it no rows are built. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UScrollBox* OptionList;

    /** @brief Option widget class created for each entry. Set in the Blueprint defaults; without it no rows are built. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dropdown")
    TSubclassOf<UDropdownOptionWidget> OptionClass;

    /**
     * @brief Binds the header button's click, hover and press events and shows "None" as the selection.
     * @return False if the base class fails to initialize.
     */
    virtual bool Initialize() override;

    /**
     * @brief Stores the header button colors and colors the popup, labels and arrow.
     * @param Theme  Active theme. Ignored if null.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    /** @brief Toggles the popup. */
    UFUNCTION()
    void HandleDropdownButtonClicked();

    /**
     * @brief Forwards an option click to SelectType().
     * @param TypeKey            Type key of the clicked option.
     * @param OptionDisplayName  Display name sent by the option.
     */
    UFUNCTION()
    void HandleOptionClicked(const FString& TypeKey, const FString& OptionDisplayName);

    /** @brief Tints the header button with the hover color unless the popup is open. */
    UFUNCTION()
    void HandleDropdownButtonHovered();

    /** @brief Restores the normal header color unless the popup is open. */
    UFUNCTION()
    void HandleDropdownButtonUnhovered();

    /** @brief Tints the header button with the pressed color. */
    UFUNCTION()
    void HandleDropdownButtonPressed();

    /**
     * @brief Creates one option row and adds it to OptionList.
     *
     * Reads the display name and the no-server-handling flag from UDT4MOBEntityFactory and
     * binds the row with RegisterOption().
     *
     * @param TypeKeyToAdd  Type key for the row. Empty creates the "None" row.
     */
    void AddOptionChild(const FString& TypeKeyToAdd);

    /** @brief Sets the header background to the hover color while the popup is open, otherwise to the normal color. */
    void RefreshButtonBackground();

    /** @brief Currently selected type key. Empty means None. */
    FString SelectedType;

    /** @brief Type keys from the last PopulateTypes() call. */
    TArray<FString> AvailableTypes;

    /** @brief True while the popup is open. */
    bool bIsOpen = false;

    /** @brief Header background when idle (theme ButtonIdle). */
    FLinearColor ButtonNormalColor = FLinearColor::Transparent;

    /** @brief Header background while hovered or while the popup is open (theme Hover). */
    FLinearColor ButtonHoverColor = FLinearColor::Transparent;

    /** @brief Header background while pressed (theme Pressed). */
    FLinearColor ButtonPressedColor = FLinearColor::Transparent;
};
