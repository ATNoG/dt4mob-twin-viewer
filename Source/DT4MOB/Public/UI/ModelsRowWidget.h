/**
 * @file
 * @brief Declares UModelsRowWidget, one mesh-layer row in the Models tab, and its toggle delegates.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "ModelsRowWidget.generated.h"

class ATempUIActor;
class UTextBlock;
class UButton;
class UBorder;

/**
 * @brief Fires after the user toggles a mesh layer's visibility from a UModelsRowWidget.
 * @param LayerName  Mesh layer that changed.
 * @param bVisible   New visibility of the layer.
 * @ingroup ui
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLayerVisibilityToggled, const FString&, LayerName, bool, bVisible);

/**
 * @brief Fires after the user clicks the transparency button of a UModelsRowWidget.
 * @param LayerName     Mesh layer that was toggled.
 * @param bTranslucent  Ghost material state after the click. Unchanged from before if no ghost
 *                      material is configured in UMeshVisualSettings.
 * @ingroup ui
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLayerTransparencyToggled, const FString&, LayerName, bool, bTranslucent);

/**
 * @brief One row in the Models tab for a single mesh layer of an ATempUIActor, with a visibility
 *        toggle and an optional transparency toggle.
 *
 * UModelsTabWidget creates one for each ungrouped layer, and UModelsGroupRowWidget creates an
 * indented one for each member of a group.
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UModelsRowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Binds the toggle and transparency buttons and gives them the pill button style.
     * @return False if the base UUserWidget::Initialize() fails, true otherwise.
     */
    virtual bool Initialize() override;

    /**
     * @brief Binds the row to one mesh layer and refreshes its labels.
     * @param Actor      Actor that owns the layer.
     * @param LayerName  Key of the layer in ATempUIActor::MeshLayers.
     */
    void SetEntry(ATempUIActor* Actor, const FString& LayerName);

    /**
     * @brief Sets the zebra striping state of the row.
     *
     * Even rows use the theme's Hover color (a translucent overlay), odd rows are transparent.
     *
     * @param bEven  True for an even row.
     */
    void SetEvenRow(bool bEven);

    /**
     * @brief Visually nests the row under a group header.
     *
     * Shifts LayerNameLabel right by IndentPxPerLevel per level with a render translation (the
     * layout is unchanged) and shrinks its font by FontShrinkPerLevel points per level, down to
     * a minimum size of 1.
     *
     * @param Level  Nesting depth; 0 leaves the row as authored.
     * @note The font reduction starts from the label's current size, so calling it twice shrinks the font twice.
     */
    void SetIndentLevel(int32 Level);

    /** @brief Broadcast after the visibility button changes the layer's visibility. */
    UPROPERTY(BlueprintAssignable)
    FOnLayerVisibilityToggled OnVisibilityToggled;

    /** @brief Broadcast after the transparency button is clicked, with the layer's resulting state. */
    UPROPERTY(BlueprintAssignable)
    FOnLayerTransparencyToggled OnTransparencyToggled;

protected:
    /** @brief Shows the layer name. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* LayerNameLabel;

    /** @brief Toggles the layer's visibility. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* ToggleButton;

    /** @brief Reads "ON" or "OFF" for the layer's visibility. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ToggleLabel;

    /** @brief Switches the layer between its original materials and the ghost material from UMeshVisualSettings. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* TransparencyButton;

    /** @brief Reads "TRANSPARENT" or "OPAQUE" for the layer's material state. Optional. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* TransparencyLabel;

    /** @brief Row background used for striping. Optional; must be named "Border" in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Border;

    /**
     * @brief Takes the striping colors from Theme and repaints the row background.
     * @param Theme  Active theme; nothing changes if it is null.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    /** @brief Actor that owns the layer. Set by SetEntry(). */
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    /** @brief Layer name passed to SetEntry(). */
    FString CachedLayerName;

    /** @brief Striping state set by SetEvenRow(). */
    bool bIsEvenRow = true;
    /** @brief Background of even rows (theme Hover). */
    FLinearColor EvenRowColor = FLinearColor::Transparent;
    /** @brief Background of odd rows (always transparent). */
    FLinearColor OddRowColor = FLinearColor::Transparent;

    /** @brief Horizontal shift of LayerNameLabel per indent level, in Slate units. */
    static constexpr float IndentPxPerLevel = 16.f;

    /** @brief Font size reduction of LayerNameLabel per indent level, in points. */
    static constexpr int32 FontShrinkPerLevel = 1;

    /** @brief Paints Border with the even or odd row color. */
    void RefreshRowBackground();
    /** @brief Updates ToggleLabel from the layer's current visibility. */
    void RefreshToggleLabel();
    /** @brief Updates TransparencyLabel from the layer's current material state. */
    void RefreshTransparencyLabel();

    /**
     * @brief Flips the layer's visibility, updates ToggleLabel and broadcasts OnVisibilityToggled.
     * @note ATempUIActor::SetMeshLayerVisible() broadcasts OnMeshLayersChanged, so the owning
     *       UModelsTabWidget rebuilds its list and replaces this row.
     */
    UFUNCTION()
    void HandleToggleClicked();

    /** @brief Flips the ghost material on the layer, updates TransparencyLabel and broadcasts OnTransparencyToggled. */
    UFUNCTION()
    void HandleTransparencyClicked();
};
