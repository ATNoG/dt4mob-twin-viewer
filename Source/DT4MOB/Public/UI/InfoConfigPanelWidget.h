/**
 * @file
 * @brief Declares UInfoConfigPanelWidget, the panel that picks which fields the Info tab shows.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/InfoFieldTypes.h"
#include "InfoConfigPanelWidget.generated.h"

class ATempUIActor;
class UInfoFieldRegistry;
class UScrollBox;
class UButton;
class UBorder;
class UCheckBox;

/** @brief Fired when the panel should close, after Save or Reset. @ingroup ui */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConfigPanelClosed);

/**
 * @brief Panel for choosing which JSON fields the Info tab shows for one entity type.
 *
 * Lists every leaf field of the bound entity's JSON, plus the type's default fields that this
 * entity lacks (shown dimmed). Each row has a checkbox; Save writes the checked fields to
 * UInfoFieldRegistry for the type, Reset restores the type's defaults. Both close the panel.
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UInfoConfigPanelWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Binds the Save, Reset and Select All controls.
     * @return False if the base widget failed to initialise.
     */
    virtual bool Initialize() override;

    /**
     * @brief Fills the panel for an entity and builds the rows.
     * @param Actor    Entity whose JSON provides the candidate fields. Must not be null.
     * @param TypeKey  Entity type whose field selection is edited.
     * @param Registry Registry that stores the selection.
     */
    void Setup(ATempUIActor* Actor, const FString& TypeKey, UInfoFieldRegistry* Registry);

    /** @brief Fired after Save or Reset so the owner can close the panel. */
    UPROPERTY(BlueprintAssignable)
    FOnConfigPanelClosed OnClosed;

protected:
    /**
     * @brief Makes the buttons' own backgrounds transparent and colours their borders from the theme.
     * @param Theme Active theme.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

    /** @brief Scroll box that holds one row per candidate field. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UScrollBox* CandidateList;

    /** @brief Saves the checked fields. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* SaveBtn;

    /** @brief Optional border behind SaveBtn that shows hover and press colours. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* SaveBtnBorder;

    /** @brief Restores the type's default fields. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* ResetBtn;

    /** @brief Optional border behind ResetBtn that shows hover and press colours. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* ResetBtnBorder;

    /** @brief Optional checkbox that checks or clears every row. Shows the undetermined state when only some rows are checked. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UCheckBox* SelectAllCB;

private:
    /** @brief Entity type passed to Setup(). */
    FString CachedTypeKey;

    /** @brief Registry passed to Setup(). */
    UPROPERTY()
    UInfoFieldRegistry* CachedRegistry = nullptr;

    /** @brief Candidate fields, in row order. Index i matches CheckBoxes[i]. */
    TArray<FInfoFieldCandidate> Candidates;

    /** @brief Row checkboxes, in the same order as Candidates. */
    UPROPERTY()
    TArray<UCheckBox*> CheckBoxes;

    /** @brief Set while Select All changes every row, so the per-row handler does not recompute the Select All state. */
    bool bUpdatingSelectAll = false;

    /** @brief Rebuilds the rows from Candidates, checking the ones currently selected in the registry. */
    void BuildRows();

    /** @brief Sets SelectAllCB to checked, unchecked or undetermined from the row checkboxes. */
    void UpdateSelectAllState();

    /** @brief Writes the checked rows to the registry for the type and fires OnClosed. */
    UFUNCTION() void HandleSave();
    UFUNCTION() void HandleSaveBtnHovered();    ///< Sets the Save border to the hover colour.
    UFUNCTION() void HandleSaveBtnUnhovered();  ///< Sets the Save border back to the normal colour.
    UFUNCTION() void HandleSaveBtnPressed();    ///< Sets the Save border to the pressed colour.
    UFUNCTION() void HandleSaveBtnReleased();   ///< Sets the Save border back to the hover colour.

    /** @brief Resets the type to its default fields and fires OnClosed. */
    UFUNCTION() void HandleReset();
    UFUNCTION() void HandleResetBtnHovered();   ///< Sets the Reset border to the hover colour.
    UFUNCTION() void HandleResetBtnUnhovered(); ///< Sets the Reset border back to the normal colour.
    UFUNCTION() void HandleResetBtnPressed();   ///< Sets the Reset border to the pressed colour.
    UFUNCTION() void HandleResetBtnReleased();  ///< Sets the Reset border back to the hover colour.

    /**
     * @brief Checks or clears every row.
     * @param bIsChecked New state for all rows.
     */
    UFUNCTION() void HandleSelectAllChanged(bool bIsChecked);

    /**
     * @brief Updates the Select All state after a row changes.
     * @param bIsChecked New state of the row (unused).
     */
    UFUNCTION() void HandleRowCheckChanged(bool bIsChecked);

    FLinearColor SaveBorderNormal;   ///< Save border colour at rest, from the theme.
    FLinearColor SaveBorderHovered;  ///< Save border colour on hover, from the theme.
    FLinearColor SaveBorderPressed;  ///< Save border colour while pressed, from the theme.

    FLinearColor ResetBorderNormal;  ///< Reset border colour at rest, from the theme.
    FLinearColor ResetBorderHovered; ///< Reset border colour on hover, from the theme.
    FLinearColor ResetBorderPressed; ///< Reset border colour while pressed, from the theme.
};
