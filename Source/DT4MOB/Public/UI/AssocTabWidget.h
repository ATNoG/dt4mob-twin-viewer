/**
 * @file
 * @brief Declares UAssocTabWidget, the Associated tab of the entity window.
 * @ingroup ui
 */

#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/AssocRowWidget.h"
#include "AssocTabWidget.generated.h"

class ATempUIActor;
class UVerticalBox;
class UTextBlock;

/**
 * @brief Associated tab of the entity window: lists the instrument things that belong to the bound entity.
 *
 * An associated entity is any actor in UActorRegistryService whose thingId starts with the
 * bound entity's thingId followed by `.instrument.`. The list is rebuilt when the bound
 * actor changes and whenever a matching actor is registered or unregistered. Clicking a
 * row's open button selects that entity through USelectionManager, which makes the HUD open
 * or focus its window.
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UAssocTabWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /** @brief Subscribes to the actor registry's register and unregister events. */
    virtual void NativeConstruct() override;

    /** @brief Unsubscribes from the actor registry and drops the bound actor. */
    virtual void NativeDestruct() override;

    /**
     * @brief Sets the entity whose associated things are listed and rebuilds the list.
     * @param Actor  Entity shown in the window. Null clears the list and shows a count of 0.
     */
    UFUNCTION(BlueprintCallable, Category = "AssocTab")
    void SetBoundActor(ATempUIActor* Actor);

protected:
    /**
     * @brief Colors the count label with the theme's primary text color.
     * @param Theme  Active theme. Ignored if null.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

    /** @brief List populated with one UAssocRowWidget per associated actor. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* AssocList;

    /** @brief Header text that shows "ASSOCIATED (N)". Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* CountLabel;

    /** @brief Row widget class created for each associated actor. Set in the WBP_AssocTabWidget defaults; without it the list stays empty. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AssocTab")
    TSubclassOf<UAssocRowWidget> RowClass;

private:
    /** @brief Entity whose associated things are listed. */
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    /** @brief Clears the list, updates the count and creates one striped row per associated actor. */
    void RebuildList();

    /**
     * @brief Selects the actor with the given thingId so the HUD opens its window.
     * @param ThingId  thingId sent by the clicked row. Ignored if no such actor is registered.
     */
    UFUNCTION()
    void HandleOpenRequested(const FString& ThingId);

    /**
     * @brief Rebuilds the list if the new actor is an instrument of the bound entity.
     * @param Actor  Actor that was just registered.
     */
    UFUNCTION()
    void HandleEntityRegistered(ATempUIActor* Actor);

    /**
     * @brief Rebuilds the list if the removed actor was an instrument of the bound entity.
     * @param ThingId  thingId of the actor that was unregistered.
     */
    UFUNCTION()
    void HandleEntityUnregistered(const FString& ThingId);
};
