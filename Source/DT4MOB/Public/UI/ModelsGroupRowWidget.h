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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGroupExpandedChanged, const FString&, GroupName, bool, bExpanded);

/**
 * @brief Collapsible header row for a mesh-layer group (see ATempUIActor::MeshLayerGroups) —
 *        one entry per source model (e.g. "Cone") instead of one per individual mesh node.
 *        Same visibility/transparency toggles as UModelsRowWidget, but acting on the whole
 *        group (ATempUIActor::SetLayerGroupVisible/SetLayerGroupTranslucent), plus an expand
 *        control that reveals the group's individual member rows (reusing UModelsRowWidget
 *        unchanged for each one).
 */
UCLASS()
class DT4MOB_API UModelsGroupRowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    /** Binds this row to GroupName on Actor and populates its (initially hidden) child rows. */
    void SetEntry(ATempUIActor* Actor, const FString& InGroupName, TSubclassOf<UModelsRowWidget> ChildRowClass);

    /** @brief Sets expand/collapse state directly (e.g. to restore it after the owning list rebuilds
     *  this row from scratch) without going through the click handler or firing OnExpandedChanged. */
    void SetExpanded(bool bExpanded);

    bool IsExpanded() const { return bIsExpanded; }

    /** @brief Fired whenever the user toggles this group's expand state, so the owning list
     *  (ModelsTabWidget) can remember it across a full rebuild — see SetExpanded(). */
    UPROPERTY(BlueprintAssignable)
    FOnGroupExpandedChanged OnExpandedChanged;

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* LayerNameLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* ToggleButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ToggleLabel;

    /** Optional — toggles every member layer between its original material and the ghost material. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* TransparencyButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* TransparencyLabel;

    /** Row background. Must be named "Border" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Border;

    /** Expand/collapse header button. Must be named "ExpandButton" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* ExpandButton;

    /** Expand/collapse chevron icon. Must be named "Arrow" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* Arrow;

    /** Container for the group's individual member rows. Must be named "ChildList" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* ChildList;

    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    FString GroupName;
    bool bIsExpanded = false;

    void RefreshToggleLabel();
    void RefreshTransparencyLabel();
    void PopulateChildren(TSubclassOf<UModelsRowWidget> ChildRowClass);

    UFUNCTION()
    void HandleToggleClicked();

    UFUNCTION()
    void HandleTransparencyClicked();

    UFUNCTION()
    void HandleExpandClicked();
};
