/** @file UnifiedController.cpp
 *  @brief Implementation of AUnifiedController. All logic documentation is in the header.
 */
#include "Gameplay/UnifiedPawn/UnifiedController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Gameplay/UnifiedPawn/UnifiedPawn.h"
#include "UI/RootHUDWidget.h"
#include "GameFramework/PlayerController.h"
#include "Managers/SelectionManager.h"

void AUnifiedController::ApplyGameInputMode(ECameraMode NewMode)
{
    UE_LOG(LogTemp, Warning, TEXT("Applying game input mode: %d"), static_cast<int32>(NewMode));
    if (NewMode == ECameraMode::RTS)
    {
        // RTS always uses mouse interaction
        FInputModeGameAndUI InputMode;
        SetInputMode(InputMode);
        bShowMouseCursor = true;

        // Leaving FreeFly interaction state
        bFreeFlyMouseUnlocked = false;
    }
    else
    {
        // FreeFly defaults to locked mouse-look unless explicitly unlocked
        if (bFreeFlyMouseUnlocked)
        {
            FInputModeGameAndUI InputMode;
            SetInputMode(InputMode);
            bShowMouseCursor = true;
        }
        else
        {
            FInputModeGameOnly InputMode;
            SetInputMode(InputMode);
            bShowMouseCursor = false;
        }
    }
}

void AUnifiedController::BeginPlay()
{
    Super::BeginPlay();

    if (ULocalPlayer *LP = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
                LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (MappingContext)
            {
                Subsystem->AddMappingContext(MappingContext, 0);
            }
        }

        SelectionManager = LP->GetSubsystem<USelectionManager>();
    }

    if (AUnifiedPawn *Cam = Cast<AUnifiedPawn>(GetPawn()))
    {
        ApplyGameInputMode(Cam->GetCameraMode());
    }

    if (HUDWidgetClass)
    {
        UUserWidget *HUD = CreateWidget<UUserWidget>(this, HUDWidgetClass);
        if (HUD)
        {
            HUD->AddToViewport();
        }
    }
}

void AUnifiedController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateHover();
}
void AUnifiedController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent *EIC =
            CastChecked<UEnhancedInputComponent>(InputComponent))
    {
        if (MoveAction)
        {
            EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AUnifiedController::Move);
            EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &AUnifiedController::Move);
            EIC->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AUnifiedController::Move);
        }

        if (LookAction)
        {
            EIC->BindAction(LookAction, ETriggerEvent::Triggered, this,
                            &AUnifiedController::Look);
        }

        if (ZoomAction)
        {
            EIC->BindAction(ZoomAction, ETriggerEvent::Triggered, this,
                            &AUnifiedController::Zoom);
        }
        if (LeftClickAction)
        {
            EIC->BindAction(LeftClickAction, ETriggerEvent::Started, this,
                            &AUnifiedController::LeftClick);
        }
        if (VerticalMoveAction)
        {
            EIC->BindAction(VerticalMoveAction, ETriggerEvent::Triggered, this, &AUnifiedController::VerticalMove);
            EIC->BindAction(VerticalMoveAction, ETriggerEvent::Completed, this, &AUnifiedController::VerticalMove);
            EIC->BindAction(VerticalMoveAction, ETriggerEvent::Canceled, this, &AUnifiedController::VerticalMove);
        }
        if (ToggleMouseUnlockAction)
        {
            EIC->BindAction(ToggleMouseUnlockAction, ETriggerEvent::Started, this, &AUnifiedController::ToggleFreeFlyMouseUnlock);
        }
    }
}

void AUnifiedController::Move(const FInputActionValue &Value)
{
    if (AUnifiedPawn *ControlledPawn = Cast<AUnifiedPawn>(GetPawn()))
    {
        ControlledPawn->HandleMove(Value);
    }
}

void AUnifiedController::Look(const FInputActionValue &Value)
{
    if (AUnifiedPawn *Cam = Cast<AUnifiedPawn>(GetPawn()))
    {
        if (Cam->GetCameraMode() == ECameraMode::FreeFly && bFreeFlyMouseUnlocked)
        {
            return;
        }

        Cam->HandleLook(Value);
    }
}

void AUnifiedController::Zoom(const FInputActionValue &Value)
{
    if (AUnifiedPawn *ControlledPawn = Cast<AUnifiedPawn>(GetPawn()))
    {
        ControlledPawn->HandleZoom(Value);
    }
}

void AUnifiedController::LeftClick(const FInputActionValue &Value)
{
    if (!SelectionManager)
    {
        return;
    }

    AUnifiedPawn *CamPawn = Cast<AUnifiedPawn>(GetPawn());
    if (!CamPawn)
    {
        return;
    }

    const ECameraMode Mode = CamPawn->GetCameraMode();

    const bool bCanUseCursorInteraction =
        (Mode == ECameraMode::RTS) ||
        (Mode == ECameraMode::FreeFly && bFreeFlyMouseUnlocked);

    if (!bCanUseCursorInteraction)
    {
        return;
    }

    FHitResult Hit;
    if (TraceFromCursor(Hit))
    {
        SelectionManager->SetSelectedActor(Hit.GetActor());
    }
    else
    {
        SelectionManager->SetSelectedActor(nullptr);
    }
}

void AUnifiedController::VerticalMove(const FInputActionValue &Value)
{
    if (AUnifiedPawn *ControlledPawn = Cast<AUnifiedPawn>(GetPawn()))
    {
        ControlledPawn->HandleVerticalMove(Value);
    }
}

void AUnifiedController::ToggleCameraMode()
{
    if (AUnifiedPawn *ControlledPawn = Cast<AUnifiedPawn>(GetPawn()))
    {
        const ECameraMode NewMode =
            (ControlledPawn->GetCameraMode() == ECameraMode::RTS)
                ? ECameraMode::FreeFly
                : ECameraMode::RTS;

        ControlledPawn->SetCameraMode(NewMode);

        // Reset FreeFly mouse unlock when changing modes
        if (NewMode != ECameraMode::FreeFly)
        {
            bFreeFlyMouseUnlocked = false;
        }

        ApplyGameInputMode(NewMode);
    }
}

void AUnifiedController::ToggleFreeFlyMouseUnlock()
{
    if (AUnifiedPawn *CamPawn = Cast<AUnifiedPawn>(GetPawn()))
    {
        if (CamPawn->GetCameraMode() != ECameraMode::FreeFly)
        {
            return;
        }
    }

    SetFreeFlyMouseUnlocked(!bFreeFlyMouseUnlocked);
}

void AUnifiedController::SetFreeFlyMouseUnlocked(bool bUnlocked)
{
    AUnifiedPawn *Cam = Cast<AUnifiedPawn>(GetPawn());
    if (!Cam || Cam->GetCameraMode() != ECameraMode::FreeFly)
    {
        return;
    }

    bFreeFlyMouseUnlocked = bUnlocked;

    if (bFreeFlyMouseUnlocked)
    {
        FInputModeGameAndUI InputMode;
        SetInputMode(InputMode);
        bShowMouseCursor = true;
    }
    else
    {
        FInputModeGameOnly InputMode;
        SetInputMode(InputMode);
        bShowMouseCursor = false;
    }
}

bool AUnifiedController::TraceFromCursor(FHitResult &OutHit) const
{
    FVector WorldLocation;
    FVector WorldDirection;

    if (!DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
    {
        return false;
    }

    const float TraceDistance = 500000000.f;
    const FVector TraceEnd = WorldLocation + (WorldDirection * TraceDistance);

    FCollisionQueryParams Params;
    if (const APawn *P = GetPawn())
    {
        Params.AddIgnoredActor(P);
    }

    return GetWorld()->LineTraceSingleByChannel(
        OutHit,
        WorldLocation,
        TraceEnd,
        ECC_GameTraceChannel1,
        Params);
}

void AUnifiedController::UpdateHover()
{
    if (!SelectionManager)
    {
        return;
    }

    AUnifiedPawn *CamPawn = Cast<AUnifiedPawn>(GetPawn());
    if (!CamPawn)
    {
        SelectionManager->SetHoveredActor(nullptr);
        return;
    }

    const ECameraMode Mode = CamPawn->GetCameraMode();

    const bool bCanUseCursorInteraction =
        (Mode == ECameraMode::RTS) ||
        (Mode == ECameraMode::FreeFly && bFreeFlyMouseUnlocked);

    if (!bCanUseCursorInteraction)
    {
        SelectionManager->SetHoveredActor(nullptr);
        return;
    }

    FHitResult Hit;
    if (TraceFromCursor(Hit))
    {
        SelectionManager->SetHoveredActor(Hit.GetActor());
    }
    else
    {
        SelectionManager->SetHoveredActor(nullptr);
    }
}