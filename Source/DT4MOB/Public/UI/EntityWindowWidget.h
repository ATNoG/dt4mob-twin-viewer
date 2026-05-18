// EntityWindowWidget.h
#pragma once
#include "BaseWindowWidget.h"
#include "EntityWindowWidget.generated.h"

class ATempUIActor;
class URootHUDWidget;
class UJsonViewerWidget;

/**
 * @brief HUD widget that displays the name and live JSON data of a selected ATempUIActor.
 *
 * Subscribes to ATempUIActor::OnEntityDataChanged so the displayed JSON refreshes
 * automatically when the Daemon delivers live updates to the bound actor.
 * Bound widget components (NameText, DataText) must exist in the Blueprint layout.
 */
UCLASS()
class UEntityWindowWidget : public UBaseWindowWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;   

public:
    /** @brief Text block displaying the actor's name. Must be bound in the Blueprint widget. */
    UPROPERTY(meta = (BindWidget))
    class UTextBlock *NameText;

    /** @brief Text block displaying the actor's raw JSON payload. Optional — superseded by JsonViewer when present. */
    UPROPERTY(meta = (BindWidgetOptional))
    class UTextBlock *DataText;

    /** @brief Optional collapsible JSON tree viewer. When present it replaces DataText for JSON display. */
    UPROPERTY(meta = (BindWidgetOptional))
    UJsonViewerWidget *JsonViewer;

    /**
     * @brief Binds an actor to this window and populates NameText and DataText.
     *
     * Unbinds from the previous actor's OnEntityDataChanged delegate before binding
     * to the new one. If Actor is not an ATempUIActor, DataText is cleared.
     *
     * @param Actor The actor to display, or nullptr to clear the window.
     */
    virtual void OnBindData_Implementation(AActor *Actor) override;

    /**
     * @brief Closes the window and unbinds from the currently bound actor's delegate.
     */
    virtual void CloseWindow() override;

    /**
     * @brief Called after binding an actor whose ThingId has instrument children.
     *
     * Blueprint implements this to show/populate the Instruments tab.
     * If Instruments is empty the tab should be hidden.
     *
     * @param Instruments All ATempUIActors whose ThingId starts with
     *                    BoundActor->GetThingId() + ".instrument."
     */
    UFUNCTION(BlueprintImplementableEvent)
    void OnInstrumentsLoaded(const TArray<ATempUIActor *> &Instruments);

    /**
     * @brief Called when a geo-asset (non-instrument) is selected.
     *
     * Blueprint implements this to show/populate the Overlays tab with a dropdown.
     * OverlayActors contains all level actors tagged "GeoOverlay", in tag-sort order.
     * Pass an empty array when the selected entity is not a geo-asset — Blueprint
     * should hide the Overlays tab in that case.
     */
    UFUNCTION(BlueprintImplementableEvent)
    void OnOverlaysAvailable(const TArray<AActor *> &OverlayActors);

    /** @brief Stores a reference to the owning RootHUDWidget so Blueprint can call OpenWindowForActor on it. */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void SetOwnerHUD(URootHUDWidget *HUD);

    /** @brief Returns the owning RootHUDWidget set via SetOwnerHUD. */
    UFUNCTION(BlueprintPure, Category = "UI")
    URootHUDWidget *GetOwnerHUD() const { return OwnerHUD; }

private:
    /** @brief Pointer to the actor whose data is currently being displayed. */
    UPROPERTY()
    class ATempUIActor *BoundActor = nullptr;

    /** @brief Cached reference to the RootHUDWidget that owns this window. */
    UPROPERTY()
    URootHUDWidget *OwnerHUD = nullptr;

    /**
     * @brief Called when the bound ATempUIActor's OnEntityDataChanged fires.
     *
     * Refreshes DataText with the latest JSON string from the actor.
     */
    UFUNCTION()
    void HandleDataChanged();
};
