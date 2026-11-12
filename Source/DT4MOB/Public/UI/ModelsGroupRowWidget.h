/**
 * @file
 * @brief Declares UModelsGroupRowWidget, the collapsible group header row in the Models tab.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/ModelsRowWidget.h"
#include "ModelsGroupRowWidget.generated.h"

class ATempUIActor;
class UTextBlock;
class UButton;
class UBorder;
class UImage;
class UVerticalBox;

/**
 * @brief Fires when the user expands or collapses a mesh-layer group row.
 * @param GroupName  Group whose header was clicked.
 * @param bExpanded  New expand state.
 * @ingroup ui
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGroupExpandedChanged, const FString&, GroupName, bool, bExpanded);

/**
 * @brief Collapsible header row for one mesh-layer group in the Models tab.
 *
 * A group stands for one source model (for example "Cone") that
 * ATempUIActor::AddOrReplaceMeshLayerGroup() split into several mesh layers, so the tab shows
 * one row per model instead of one per mesh node. The row has the same visibility and
 * transparency toggles as UModelsRowWidget, but they act on the whole group through
 * ATempUIActor::SetLayerGroupVisible() and ATempUIActor::SetLayerGroupTranslucent(). The expand
 * button reveals one indented UModelsRowWidget per member layer.
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UModelsGroupRowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Binds the toggle, transparency and expand buttons, gives the toggles the pill button
     *        style, centers the arrow's render pivot and applies the current expand state.
     * @return False if the base UUserWidget::Initialize() fails, true otherwise.
     */
    virtual bool Initialize() override;

    /**
     * @brief Binds the row to a group on Actor, updates the labels and creates the member rows.
     *
     * The member rows go into ChildList, which stays hidden until the row is expanded.
     *
     * @param Actor          Actor that owns the group.
     * @param InGroupName    Group name, as returned by ATempUIActor::GetMeshLayerGroupNames().
     * @param ChildRowClass  Widget class for each member row; no member rows are created if it is null.
     */
    void SetEntry(ATempUIActor* Actor, const FString& InGroupName, TSubclassOf<UModelsRowWidget> ChildRowClass);

    /**
     * @brief Shows or hides the member rows and rotates the arrow, without broadcasting OnExpandedChanged.
     *
     * UModelsTabWidget calls it to restore the expand state on a row it has just recreated.
     *
     * @param bExpanded  True to show the member rows.
     */
    void SetExpanded(bool bExpanded);

    /**
     * @brief Whether the member rows are currently shown.
     * @return True if the row is expanded.
     */
    bool IsExpanded() const { return bIsExpanded; }

    /** @brief Broadcast when the user clicks the expand button. UModelsTabWidget binds it to remember the state across list rebuilds (see SetExpanded()). */
    UPROPERTY(BlueprintAssignable)
    FOnGroupExpandedChanged OnExpandedChanged;

protected:
    /** @brief Shows the group name. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* LayerNameLabel;

    /** @brief Toggles the visibility of every layer in the group. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* ToggleButton;

    /** @brief Reads "ON" if any layer of the group is visible, otherwise "OFF". Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ToggleLabel;

    /** @brief Switches every layer of the group between its original materials and the ghost material. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* TransparencyButton;

    /** @brief Reads "TRANSPARENT" if any layer of the group has the ghost material, otherwise "OPAQUE". Optional. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* TransparencyLabel;

    /** @brief Row background, tinted with the theme's BackgroundSecondary. Optional; must be named "Border" in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Border;

    /** @brief Header button that expands or collapses the member rows. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* ExpandButton;

    /** @brief Chevron icon, rotated 0 degrees when expanded and -90 degrees when collapsed. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* Arrow;

    /** @brief Holds the member rows; collapsed while the group is collapsed. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* ChildList;

    /**
     * @brief Tints Border with the theme's BackgroundSecondary.
     * @param Theme  Active theme; nothing changes if it is null.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    /** @brief Actor that owns the group. Set by SetEntry(). */
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    /** @brief Group name on BoundActor. Set by SetEntry(). */
    FString GroupName;
    /** @brief Whether ChildList is currently shown. */
    bool bIsExpanded = false;

    /** @brief Updates ToggleLabel from the group's current visibility. */
    void RefreshToggleLabel();
    /** @brief Updates TransparencyLabel from the group's current material state. */
    void RefreshTransparencyLabel();
    /**
     * @brief Clears ChildList and adds one indented, striped row per member layer of the group.
     * @param ChildRowClass  Widget class for each member row; nothing happens if it is null.
     */
    void PopulateChildren(TSubclassOf<UModelsRowWidget> ChildRowClass);

    /**
     * @brief Flips the visibility of the whole group and updates ToggleLabel.
     * @note ATempUIActor::SetLayerGroupVisible() broadcasts OnMeshLayersChanged, so the owning
     *       UModelsTabWidget rebuilds its list and replaces this row.
     */
    UFUNCTION()
    void HandleToggleClicked();

    /** @brief Flips the ghost material on the whole group and updates TransparencyLabel. */
    UFUNCTION()
    void HandleTransparencyClicked();

    /** @brief Toggles the expand state and broadcasts OnExpandedChanged. */
    UFUNCTION()
    void HandleExpandClicked();
};
