/**
 * @file
 * @brief Declares UToolbarWidget, the main toolbar, and its delegates.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/ToolbarButtonWidget.h"
#include "UI/EntityTypeDropdownWidget.h"
#include "Components/Border.h"
#include "ToolbarWidget.generated.h"

class UPlacementManager;
class UDT4MOBEntityFactory;

/** @brief Fired when the toolbar's Outline button is clicked. @ingroup ui */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOutlineToggled);

/** @brief Fired when the selected entity type changes. @param TypeKey Selected type, or empty for none. @ingroup ui */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityTypeFilterChanged, const FString&, TypeKey);

/** @brief Fired when the toolbar's Options button is clicked. @ingroup ui */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOptionsRequested);

/**
 * @brief HUD toolbar: camera switch, entity type filter, place entity, and outline toggle.
 *
 * Populates the entity type dropdown automatically from UDT4MOBEntityFactory on init.
 * Broadcasts delegates that RootHUDWidget listens to for panel coordination.
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UToolbarWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Binds the toolbar buttons, fills the entity type dropdown from the factory, and
     *        disables the place button until a type is chosen.
     * @return False if the base widget failed to initialise.
     */
    virtual bool Initialize() override;

    /** @brief Broadcast when the Outline button is clicked. */
    UPROPERTY(BlueprintAssignable)
    FOnOutlineToggled OnOutlineToggled;

    /** @brief Broadcast when the user selects an entity type. Empty string means None (no filter). */
    UPROPERTY(BlueprintAssignable)
    FOnEntityTypeFilterChanged OnEntityTypeFilterChanged;

    /** @brief Broadcast when the Options button is clicked. RootHUDWidget owns/shows the actual modal. */
    UPROPERTY(BlueprintAssignable)
    FOnOptionsRequested OnOptionsRequested;

    /**
     * @brief Called whenever the selected entity type changes.
     * Blueprint should show or hide the no-server-handling warning image based on bShow.
     * @param bShow True if the selected type has no server-side handling.
     */
    UFUNCTION(BlueprintImplementableEvent)
    void OnServerHandlingWarningChanged(bool bShow);

protected:
    /** @brief Toggles RTS and FreeFly camera modes. Must be named "SwitchCameraButton" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UToolbarButtonWidget* SwitchCameraButton;

    /** @brief Starts or cancels placing an entity of the selected type. Must be named "PlaceEntityButton" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UToolbarButtonWidget* PlaceEntityButton;

    /** @brief Shows or hides the outline panel. Must be named "OutlineButton" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UToolbarButtonWidget* OutlineButton;

    /** @brief Icon-only "more options" button. Must be named "OptionsButton" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UToolbarButtonWidget* OptionsButton;

    /** @brief Entity type picker used for placement and filtering. Must be named "EntityTypeDropdown" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UEntityTypeDropdownWidget* EntityTypeDropdown;

    /** @brief Toolbar bar background. Must be named "ToolbarBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* ToolbarBackground;

    /** @brief Vertical separator lines between toolbar sections. Must be named "Separator" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Separator;

    /**
     * @brief Colours the toolbar background and separators from the theme.
     * @param Theme Active theme.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    /** @brief Placement manager of the owning player. */
    UPROPERTY()
    UPlacementManager* PlacementManager = nullptr;

    /** @brief Entity factory, used for the list of types and their metadata. */
    UPROPERTY()
    UDT4MOBEntityFactory* EntityFactory = nullptr;

    /** @brief Toggles the camera mode on the owning AUnifiedController. */
    UFUNCTION()
    void HandleSwitchCameraClicked();

    /** @brief Enters placement mode, or cancels it if it is already active. */
    UFUNCTION()
    void HandlePlaceEntityClicked();

    /** @brief Broadcasts OnOutlineToggled. */
    UFUNCTION()
    void HandleOutlineClicked();

    /** @brief Broadcasts OnOptionsRequested. */
    UFUNCTION()
    void HandleOptionsClicked();

    /**
     * @brief Passes the type to the placement manager, enables the place button for a non-empty
     *        type, updates the server handling warning and broadcasts OnEntityTypeFilterChanged.
     * @param TypeKey Selected type, or empty for none.
     */
    UFUNCTION()
    void HandleEntityTypeSelected(const FString& TypeKey);

    /**
     * @brief Shows the place button as active while placement mode is on.
     * @param bActive True while placing.
     */
    UFUNCTION()
    void HandlePlacementModeChanged(bool bActive);
};
