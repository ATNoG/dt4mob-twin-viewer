#include "UI/BaseWindowWidget.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Gameplay/UnifiedPawn/UnifiedController.h"

void UBaseWindowWidget::OpenWindow()
{
    SetVisibility(ESlateVisibility::Visible);
}

void UBaseWindowWidget::CloseWindow()
{
    SetVisibility(ESlateVisibility::Collapsed);
    // Safety net — release suppression if window is force-closed mid-drag
    if (AUnifiedController* PC = GetUnifiedController())
        PC->SetMovementInputSuppressed(false);
    bIsDragging = false;
    bIsResizing = false;
}

void UBaseWindowWidget::InitCanvasSlot()
{
    CanvasSlot = Cast<UCanvasPanelSlot>(Slot);
}

void UBaseWindowWidget::OnBindData_Implementation(AActor* Actor) {}

AUnifiedController* UBaseWindowWidget::GetUnifiedController() const
{
    if (APlayerController* PC = GetOwningPlayer())
        return Cast<AUnifiedController>(PC);
    return nullptr;
}

FEventReply UBaseWindowWidget::OnTitleBarMouseDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        bIsDragging = true;
        bIsResizing = false;

        if (!CanvasSlot)
            CanvasSlot = Cast<UCanvasPanelSlot>(Slot);

        if (CanvasSlot)
        {
            float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
            FVector2D MouseScreen = MouseEvent.GetScreenSpacePosition() / Scale;
            DragOffset = MouseScreen - FVector2D(CanvasSlot->GetPosition());
        }

        if (AUnifiedController* PC = GetUnifiedController())
            PC->SetMovementInputSuppressed(true);

        FEventReply Reply = UWidgetBlueprintLibrary::Handled();
        return UWidgetBlueprintLibrary::CaptureMouse(Reply, this);
    }
    return UWidgetBlueprintLibrary::Unhandled();
}

FEventReply UBaseWindowWidget::OnResizeHandleMouseDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
    UE_LOG(LogTemp, Log, TEXT("Resize handle mouse down: button=%s"), *MouseEvent.GetEffectingButton().ToString());
    if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && SizeBox)
    {
        UE_LOG(LogTemp, Log, TEXT("Starting resize operation"));
        bIsResizing = true;
        bIsDragging = false;
        ResizeStartMouse = MouseEvent.GetScreenSpacePosition();
        ResizeStartSize = FVector2D(SizeBox->GetWidthOverride(), SizeBox->GetHeightOverride());

        if (AUnifiedController* PC = GetUnifiedController())
            PC->SetMovementInputSuppressed(true);

        FEventReply Reply = UWidgetBlueprintLibrary::Handled();
        return UWidgetBlueprintLibrary::CaptureMouse(Reply, this);
    }
    return UWidgetBlueprintLibrary::Unhandled();
}

FReply UBaseWindowWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    UE_LOG(LogTemp, Warning, TEXT("NativeOnMouseButtonDown called"));
    
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && SizeBox)
    {
        FVector2D LocalMouse = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
        FVector2D WidgetSize = InGeometry.GetLocalSize();
        FVector2D CornerZone = WidgetSize - FVector2D(32.f, 32.f);

        UE_LOG(LogTemp, Warning, TEXT("LocalMouse: %s, WidgetSize: %s, CornerZone: %s"), 
            *LocalMouse.ToString(), *WidgetSize.ToString(), *CornerZone.ToString());

        if (LocalMouse.X >= CornerZone.X && LocalMouse.Y >= CornerZone.Y)
        {
            UE_LOG(LogTemp, Warning, TEXT("Starting resize"));
            bIsResizing = true;
            bIsDragging = false;

            float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
            ResizeStartMouse = InMouseEvent.GetScreenSpacePosition() / Scale;
            ResizeStartSize = FVector2D(SizeBox->GetWidthOverride(), SizeBox->GetHeightOverride());
            if (AUnifiedController* PC = GetUnifiedController())
                PC->SetMovementInputSuppressed(true);

            return FReply::Handled().CaptureMouse(GetCachedWidget().ToSharedRef());
        }
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FEventReply UBaseWindowWidget::OnResizeHandleMouseMove(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
    if (bIsResizing && SizeBox)
    {
        float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
        FVector2D MouseScreen = MouseEvent.GetScreenSpacePosition() / Scale;
        FVector2D Delta = MouseScreen - ResizeStartMouse;
        SizeBox->SetWidthOverride(FMath::Max(ResizeStartSize.X + Delta.X, MinWindowSize.X));
        SizeBox->SetHeightOverride(FMath::Max(ResizeStartSize.Y + Delta.Y, MinWindowSize.Y));
        return UWidgetBlueprintLibrary::Handled();
    }
    return UWidgetBlueprintLibrary::Unhandled();
}

FReply UBaseWindowWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    UE_LOG(LogTemp, Warning, TEXT("NativeOnMouseMove called: IsDragging=%s, IsResizing=%s"), 
        bIsDragging ? TEXT("true") : TEXT("false"), 
        bIsResizing ? TEXT("true") : TEXT("false"));

    if (bIsDragging && CanvasSlot)
    {
        UE_LOG(LogTemp, Warning, TEXT("Dragging window"));
        float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
        FVector2D MouseScreen = InMouseEvent.GetScreenSpacePosition() / Scale;
        FVector2D NewPos = MouseScreen - DragOffset;
        CanvasSlot->SetPosition(NewPos);
        return FReply::Handled();
    }

    if (bIsResizing && SizeBox)
    {
        float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
        FVector2D MouseScreen = InMouseEvent.GetScreenSpacePosition() / Scale;
        FVector2D Delta = MouseScreen - ResizeStartMouse;

        UE_LOG(LogTemp, Warning, TEXT("Scale: %f, MouseScreen: %s, ResizeStartMouse: %s, Delta: %s, StartSize: %s"), 
            Scale, *MouseScreen.ToString(), *ResizeStartMouse.ToString(), *Delta.ToString(), *ResizeStartSize.ToString());

        ResizeStartMouse = MouseScreen;
        ResizeStartSize.X = FMath::Max(ResizeStartSize.X + Delta.X, MinWindowSize.X);
        ResizeStartSize.Y = FMath::Max(ResizeStartSize.Y + Delta.Y, MinWindowSize.Y);
        SizeBox->SetWidthOverride(ResizeStartSize.X);
        SizeBox->SetHeightOverride(ResizeStartSize.Y);
        SizeBox->InvalidateLayoutAndVolatility();
        ForceLayoutPrepass();
        return FReply::Handled();
    }

    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}



FReply UBaseWindowWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        bIsDragging = false;
        bIsResizing = false;

        if (AUnifiedController* PC = GetUnifiedController())
            PC->SetMovementInputSuppressed(false);

        return FReply::Handled().ReleaseMouseCapture();
    }
    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

FCursorReply UBaseWindowWidget::NativeOnCursorQuery(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    // If already resizing, keep the resize cursor
    if (bIsResizing)
        return FCursorReply::Cursor(EMouseCursor::ResizeSouthEast);

    // Check if mouse is hovering over the resize corner zone
    FVector2D LocalMouse = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
    FVector2D WidgetSize = InGeometry.GetLocalSize();
    FVector2D CornerZone = WidgetSize - FVector2D(32.f, 32.f);

    if (LocalMouse.X >= CornerZone.X && LocalMouse.Y >= CornerZone.Y)
        return FCursorReply::Cursor(EMouseCursor::ResizeSouthEast);

    return Super::NativeOnCursorQuery(InGeometry, InMouseEvent);
}
