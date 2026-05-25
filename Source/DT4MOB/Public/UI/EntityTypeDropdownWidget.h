#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "EntityTypeDropdownWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityTypeSelected, const FString&, TypeKey);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDropdownStateChanged, bool, bOpen);

/**
 * @brief Custom entity-type filter dropdown for the toolbar.
 *
 * C++ owns all state (selected type, open/closed, available types).
 * Each action is a BlueprintNativeEvent — C++ handles state in _Implementation,
 * Blueprint overrides to drive the visual response (animate popup, highlight rows, etc.)
 * and calls the parent to keep state in sync.
 */
UCLASS()
class DT4MOB_API UEntityTypeDropdownWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /** Fires when the user picks a type. Empty string means no filter (None). */
    UPROPERTY(BlueprintAssignable)
    FOnEntityTypeSelected OnTypeSelected;

    /** Fires whenever the dropdown opens or closes. Blueprint should bind here to drive visuals. */
    UPROPERTY(BlueprintAssignable)
    FOnDropdownStateChanged OnDropdownStateChanged;

    /**
     * @brief Populates the dropdown with the provided type keys.
     * "None" is always prepended automatically.
     * Blueprint override should build the option list UI, then call the parent.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void PopulateTypes(const TArray<FString>& TypeKeys);

    /**
     * @brief Selects a type, closes the popup, and broadcasts OnTypeSelected.
     * Blueprint override should update highlighted option row, then call the parent.
     * Pass an empty string to select None.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void SelectType(const FString& TypeKey);

    /**
     * @brief Toggles the popup open or closed.
     * Blueprint override should animate/show/hide the popup panel, then call the parent.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void ToggleDropdown();

    /**
     * @brief Closes the popup without changing the selection.
     * Blueprint override should hide the popup panel, then call the parent.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
    void CloseDropdown();

    /** Returns the currently selected type key. Empty string means None (no filter). */
    UFUNCTION(BlueprintPure)
    FString GetSelectedType() const { return SelectedType; }

    /** Returns all available type keys (not including None). */
    UFUNCTION(BlueprintPure)
    TArray<FString> GetAvailableTypes() const { return AvailableTypes; }

    /** Returns true if the popup is currently open. */
    UFUNCTION(BlueprintPure)
    bool IsDropdownOpen() const { return bIsOpen; }

protected:
    /** The clickable header button ("Type  Car  ▾"). Must be named "DropdownButton". */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* DropdownButton;

    /** Displays the currently selected type in the header. Must be named "SelectedTypeText". */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* SelectedTypeText;

    virtual bool Initialize() override;

private:
    UFUNCTION()
    void HandleDropdownButtonClicked();

    FString SelectedType;
    TArray<FString> AvailableTypes;
    bool bIsOpen = false;
};
