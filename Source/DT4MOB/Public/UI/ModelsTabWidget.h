/**
 * @file
 * @brief Declares UModelsTabWidget, the entity window tab that lists an actor's mesh layers.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/ModelsRowWidget.h"
#include "UI/ModelsGroupRowWidget.h"
#include "ModelsTabWidget.generated.h"

class ATempUIActor;
class UVerticalBox;
class UTextBlock;

/**
 * @brief "Models" tab of the entity window: lists the mesh layers of one ATempUIActor with
 *        visibility and transparency toggles.
 *
 * Each mesh-layer group with more than one member (see ATempUIActor::AddOrReplaceMeshLayerGroup())
 * gets a collapsible UModelsGroupRowWidget; every other layer gets a plain UModelsRowWidget. The
 * whole list is rebuilt whenever the actor broadcasts ATempUIActor::OnMeshLayersChanged.
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UModelsTabWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /** @brief Calls SetBoundActor(nullptr) so the actor's OnMeshLayersChanged no longer targets this widget. */
    virtual void NativeDestruct() override;

    /**
     * @brief Shows the mesh layers of Actor and rebuilds the list.
     *
     * Moves the OnMeshLayersChanged subscription from the previous actor to Actor. The remembered
     * group expand states are cleared when Actor differs from the current one.
     *
     * @param Actor  Actor whose layers to list; null leaves the list empty.
     */
    UFUNCTION(BlueprintCallable, Category = "ModelsTab")
    void SetBoundActor(ATempUIActor* Actor);

protected:
    /**
     * @brief Colors SectionLabel with the theme's TextPrimary.
     * @param Theme  Active theme; nothing changes if it is null.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

    /** @brief Vertical box that receives the group and layer rows. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* LayerList;

    /** @brief Heading that reads "MESH LAYERS (n)", where n is the actor's total layer count. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* SectionLabel;

    /** @brief Widget class created for each layer row, including group members. Set in the Blueprint defaults; if unset, the list stays empty. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ModelsTab")
    TSubclassOf<UModelsRowWidget> RowClass;

    /**
     * @brief Widget class created for each multi-member mesh-layer group (for example "Cone"). Set in the Blueprint defaults.
     *
     * If unset, no group rows are created, and layers that belong to a multi-member group are not listed at all.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ModelsTab")
    TSubclassOf<UModelsGroupRowWidget> GroupRowClass;

private:
    /** @brief Actor whose mesh layers are listed. Null when nothing is bound. */
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    /**
     * @brief Names of the groups the user has expanded.
     *
     * RebuildList() recreates every group row (for example after a visibility toggle makes the
     * actor broadcast OnMeshLayersChanged), so the expand state is kept here and restored on the
     * new rows instead of every group collapsing again.
     */
    TSet<FString> ExpandedGroups;

    /**
     * @brief Clears LayerList and fills it again from BoundActor.
     *
     * Adds one UModelsGroupRowWidget per group with more than one member, then one
     * UModelsRowWidget per ungrouped layer, with alternating striping on the ungrouped rows.
     */
    void RebuildList();

    /** @brief Rebuilds the list when the bound actor's mesh layers change. */
    UFUNCTION()
    void HandleMeshLayersChanged();

    /**
     * @brief Records a group's expand state in ExpandedGroups.
     * @param GroupName  Group whose header was clicked.
     * @param bExpanded  New expand state.
     */
    UFUNCTION()
    void HandleGroupExpandedChanged(const FString& GroupName, bool bExpanded);
};
