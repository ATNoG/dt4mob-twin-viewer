/**
 * @file
 * @brief Declares UJsonTabWidget, the raw JSON tab of the entity window.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "JsonTabWidget.generated.h"

class ATempUIActor;
class UJsonViewerWidget;
class UBorder;

/**
 * @brief JSON tab of the entity window: the bound entity's raw JSON, refreshed on every data change.
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UJsonTabWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /** @brief Unbinds the actor. */
    virtual void NativeDestruct() override;

    /**
     * @brief Shows the given entity's JSON and follows its updates.
     * @param Actor Entity to show, or null to clear the tab.
     */
    UFUNCTION(BlueprintCallable, Category = "JsonTab")
    void SetBoundActor(ATempUIActor* Actor);

protected:
    /** @brief JSON viewer. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UJsonViewerWidget* JsonTextBox;

    /** @brief Background panel behind the JSON text, for contrast. Must be named "Border" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Border;

    /**
     * @brief Colours the background panel from the theme.
     * @param Theme Active theme.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    /** @brief Entity currently shown. */
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    /** @brief Re-reads the actor's JSON into the viewer and scrolls to the top. */
    void Refresh();

    /** @brief Calls Refresh() after the bound actor's data changed. */
    UFUNCTION()
    void HandleEntityDataChanged();
};
