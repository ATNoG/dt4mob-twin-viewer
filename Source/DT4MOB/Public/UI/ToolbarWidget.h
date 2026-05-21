#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "ToolbarWidget.generated.h"

class UPlacementManager;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOutlineToggled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityTypeFilterChanged, const FString&, TypeFilter);

/**
 * @brief HUD toolbar — Switch Camera, entity type filter, Place Entity, and Outline toggle.
 *
 * Camera toggling is handled here (moved from URootHUDWidget).
 * The Outline button broadcasts OnOutlineToggled — URootHUDWidget listens to
 * show/hide the OutlinePanel.
 * Entity type selection broadcasts OnEntityTypeFilterChanged — the gamemode
 * or OutlinePanel listens to filter the entity list.
 */
UCLASS()
class DT4MOB_API UToolbarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    /** Broadcasts when the Outline button is clicked. Listened to by URootHUDWidget. */
    UPROPERTY(BlueprintAssignable)
    FOnOutlineToggled OnOutlineToggled;

    /** Broadcasts the selected type filter string ("car", "toll", etc.) or "All". */
    UPROPERTY(BlueprintAssignable)
    FOnEntityTypeFilterChanged OnEntityTypeFilterChanged;

protected:
    /** Toggles RTS / FreeFly camera mode. Must be bound in Blueprint. */
    UPROPERTY(meta = (BindWidget))
    UButton* SwitchCameraButton;

    /** Enters / exits entity placement mode. Must be bound in Blueprint. */
    UPROPERTY(meta = (BindWidget))
    UButton* PlaceEntityButton;

    /** Toggles the OutlinePanel. Must be bound in Blueprint. */
    UPROPERTY(meta = (BindWidget))
    UButton* OutlineButton;

    /** Optional dropdown for filtering the entity list by type. Bound in Blueprint. */
    UPROPERTY(meta = (BindWidgetOptional))
    UComboBoxString* EntityTypeComboBox;

private:
    UPROPERTY()
    UPlacementManager* PlacementManager = nullptr;

    UFUNCTION()
    void HandleSwitchCameraClicked();

    UFUNCTION()
    void HandlePlaceEntityClicked();

    UFUNCTION()
    void HandleOutlineClicked();

    UFUNCTION()
    void HandleEntityTypeSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

    UFUNCTION()
    void HandlePlacementModeChanged(bool bActive);
};
