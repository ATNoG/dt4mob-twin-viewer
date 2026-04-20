// BaseWindowWidget.h
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Gameplay/UnifiedPawn/UnifiedController.h"
#include "Components/CanvasPanelSlot.h"
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

    UFUNCTION(BlueprintCallable)
    void InitCanvasSlot();

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
protected:
    UFUNCTION(BlueprintCallable)
    FEventReply OnTitleBarMouseDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

    UFUNCTION(BlueprintCallable)
    FEventReply OnResizeHandleMouseDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);
    
    UFUNCTION(BlueprintCallable)
    FEventReply OnResizeHandleMouseMove(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FCursorReply NativeOnCursorQuery(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Window")
    FVector2D MinWindowSize = FVector2D(200.f, 150.f);

    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class USizeBox* SizeBox;

private:
    bool bIsDragging = false;
    bool bIsResizing = false;
    FVector2D DragOffset;
    FVector2D ResizeStartMouse;
    FVector2D ResizeStartSize;

    UPROPERTY()
    UCanvasPanelSlot* CanvasSlot = nullptr;

    // Helper to avoid repeating the cast
    AUnifiedController* GetUnifiedController() const;

};
