#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "SelectionManager.generated.h"

/** @brief Broadcast when the selected actor changes. @param NewActor The newly selected actor, or nullptr. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectedActorChanged, AActor *, NewActor);

/** @brief Broadcast when the hovered actor changes. @param NewActor The newly hovered actor, or nullptr. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHoveredActorChanged, AActor *, NewActor);

/**
 * @brief LocalPlayer subsystem that tracks which entity actor is currently hovered or selected.
 *
 * Only ATempUIActor instances are accepted as valid hover/selection targets.
 * Any other actor type passed to SetSelectedActor() or SetHoveredActor() will be
 * treated as a deselection/clear event.
 *
 * Consumers subscribe to OnSelectedActorChanged and OnHoveredActorChanged to react
 * to state changes without polling.
 */
UCLASS()
class DT4MOB_API USelectionManager : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    /** @brief Broadcast whenever the selected actor changes. */
    UPROPERTY(BlueprintAssignable)
    FOnSelectedActorChanged OnSelectedActorChanged;

    /** @brief Broadcast whenever the hovered actor changes. */
    UPROPERTY(BlueprintAssignable)
    FOnHoveredActorChanged OnHoveredActorChanged;

    /**
     * @brief Sets the currently selected actor.
     *
     * If NewActor is not an ATempUIActor it is treated as nullptr (deselect).
     * Broadcasts OnSelectedActorChanged only if the selection actually changes.
     *
     * @param NewActor Actor to select, or nullptr to clear the selection.
     */
    void SetSelectedActor(AActor *NewActor);

    /**
     * @brief Sets the currently hovered actor.
     *
     * If NewActor is not an ATempUIActor it is treated as nullptr (clear hover).
     * Broadcasts OnHoveredActorChanged only if the hover actually changes.
     *
     * @param NewActor Actor being hovered, or nullptr to clear.
     */
    void SetHoveredActor(AActor *NewActor);

    /**
     * @brief Returns the currently selected actor.
     * @return Pointer to the selected ATempUIActor, or nullptr if nothing is selected.
     */
    AActor *GetSelectedActor() const { return SelectedActor; }

    /**
     * @brief Returns the currently hovered actor.
     * @return Pointer to the hovered ATempUIActor, or nullptr if nothing is hovered.
     */
    AActor *GetHoveredActor() const { return HoveredActor; }

private:
    /** @brief Currently selected actor. Only ATempUIActor instances are stored here. */
    UPROPERTY()
    AActor *SelectedActor = nullptr;

    /** @brief Currently hovered actor. Only ATempUIActor instances are stored here. */
    UPROPERTY()
    AActor *HoveredActor = nullptr;
};
