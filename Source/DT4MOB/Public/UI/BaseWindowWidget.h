// BaseWindowWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BaseWindowWidget.generated.h"

/**
 * @brief Abstract base class for all HUD pop-up window widgets.
 *
 * Provides a consistent open/close interface (toggling Slate visibility) and a
 * BlueprintNativeEvent hook for binding an actor's data to the window contents.
 *
 * Derived classes should override OnBindData_Implementation to populate their
 * specific UI elements.
 */
UCLASS()
class UBaseWindowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Makes the window visible by setting Slate visibility to Visible.
     *
     * Can be overridden in Blueprint or C++ to perform additional setup on open.
     */
    UFUNCTION(BlueprintCallable)
    virtual void OpenWindow();

    /**
     * @brief Hides the window by collapsing it in the Slate hierarchy.
     *
     * Can be overridden in Blueprint or C++ to perform cleanup on close.
     */
    UFUNCTION(BlueprintCallable)
    virtual void CloseWindow();

    /**
     * @brief Binds an actor's data to this window for display.
     *
     * Base implementation is a no-op. Override in derived classes to populate
     * widget fields from the actor's state.
     *
     * @param Actor The actor whose data should be shown, or nullptr to clear.
     */
    UFUNCTION(BlueprintNativeEvent)
    void OnBindData(AActor *Actor);
};
