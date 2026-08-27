#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/ToolbarButtonWidget.h"
#include "UI/EntityTypeDropdownWidget.h"
#include "Components/Border.h"
#include "ToolbarWidget.generated.h"

class UPlacementManager;
class UDT4MOBEntityFactory;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOutlineToggled);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityTypeFilterChanged, const FString&, TypeKey);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOptionsRequested);

/**
 * @brief HUD toolbar — camera switch, entity type filter, place entity, and outline toggle.
 *
 * Populates the entity type dropdown automatically from UDT4MOBEntityFactory on init.
 * Broadcasts delegates that RootHUDWidget listens to for panel coordination.
 */
UCLASS()
class DT4MOB_API UToolbarWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    /** Broadcast when the Outline button is clicked. */
    UPROPERTY(BlueprintAssignable)
    FOnOutlineToggled OnOutlineToggled;

    /** Broadcast when the user selects an entity type. Empty string means None (no filter). */
    UPROPERTY(BlueprintAssignable)
    FOnEntityTypeFilterChanged OnEntityTypeFilterChanged;

    /** Broadcast when the Options button is clicked. RootHUDWidget owns/shows the actual modal. */
    UPROPERTY(BlueprintAssignable)
    FOnOptionsRequested OnOptionsRequested;

    /**
     * @brief Called whenever the selected entity type changes.
     * Blueprint should show or hide the no-server-handling warning image based on bShow.
     */
    UFUNCTION(BlueprintImplementableEvent)
    void OnServerHandlingWarningChanged(bool bShow);

protected:
    /** Must be named "SwitchCameraButton" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UToolbarButtonWidget* SwitchCameraButton;

    /** Must be named "PlaceEntityButton" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UToolbarButtonWidget* PlaceEntityButton;

    /** Must be named "OutlineButton" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UToolbarButtonWidget* OutlineButton;

    /** Icon-only "more options" button. Must be named "OptionsButton" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UToolbarButtonWidget* OptionsButton;

    /** Must be named "EntityTypeDropdown" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UEntityTypeDropdownWidget* EntityTypeDropdown;

    /** Toolbar bar background. Must be named "ToolbarBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* ToolbarBackground;

    /** Vertical separator lines between toolbar sections. Must be named "Separator" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Separator;

    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    UPROPERTY()
    UPlacementManager* PlacementManager = nullptr;

    UPROPERTY()
    UDT4MOBEntityFactory* EntityFactory = nullptr;

    UFUNCTION()
    void HandleSwitchCameraClicked();

    UFUNCTION()
    void HandlePlaceEntityClicked();

    UFUNCTION()
    void HandleOutlineClicked();

    UFUNCTION()
    void HandleOptionsClicked();

    UFUNCTION()
    void HandleEntityTypeSelected(const FString& TypeKey);

    UFUNCTION()
    void HandlePlacementModeChanged(bool bActive);
};
