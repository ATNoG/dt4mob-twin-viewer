/** @file UnifiedMovementComponent.cpp
 *  @brief Implementation of UUnifiedMovementComponent. All logic documentation is in the header.
 */
#include "Gameplay/UnifiedPawn/UnifiedMovementComponent.h"

#include "Gameplay/UnifiedPawn/UnifiedPawn.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"

UUnifiedMovementComponent::UUnifiedMovementComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UUnifiedMovementComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UUnifiedMovementComponent::TickComponent(
    float DeltaTime,
    enum ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (CurrentMode != ECameraMode::RTS)
    {
        return;
    }

    AUnifiedPawn *UnifiedPawn = GetUnifiedPawn();
    if (!UnifiedPawn || !UnifiedPawn->SpringArm)
    {
        return;
    }

    const float CurrentArmLength = UnifiedPawn->SpringArm->TargetArmLength;
    const float TargetArmLength = FMath::FInterpTo(
        CurrentArmLength,
        UnifiedPawn->GetTargetArmLength(),
        DeltaTime,
        ZoomInterpSpeed);

    UnifiedPawn->SpringArm->TargetArmLength = TargetArmLength;
}

void UUnifiedMovementComponent::SetCameraMode(ECameraMode NewMode)
{
    CurrentMode = NewMode;
}

void UUnifiedMovementComponent::HandleMoveInput(const FVector2D &Input)
{
    if (CurrentMode == ECameraMode::RTS)
    {
        HandleRtsMove(Input);
    }
    else
    {
        HandleFreeFlyMove(Input);
    }
}

void UUnifiedMovementComponent::HandleLookInput(const FVector2D &Input)
{
    if (CurrentMode == ECameraMode::FreeFly)
    {
        HandleFreeFlyLook(Input);
    }
}

void UUnifiedMovementComponent::HandleZoomInput(float Value)
{
    if (CurrentMode == ECameraMode::RTS)
    {
        HandleRtsZoom(Value);
    }
}

void UUnifiedMovementComponent::HandleRtsMove(const FVector2D &Input)
{
    APawn *P = Cast<APawn>(GetOwner());
    AUnifiedPawn *UnifiedPawn = GetUnifiedPawn();
    if (!P || !UnifiedPawn)
    {
        return;
    }

    FVector GlobeUp;
    if (!UnifiedPawn->GetGlobeUpVector(GlobeUp))
    {
        return;
    }

    FVector North;
    if (!UnifiedPawn->GetGlobeNorthVector(North))
    {
        return;
    }

    const FVector East = FVector::CrossProduct(GlobeUp, North).GetSafeNormal();

    const float ArmLength = UnifiedPawn->SpringArm ? UnifiedPawn->SpringArm->TargetArmLength : 1000.f;
    const float CurrentPanSpeed = BasePanSpeed * ArmLength;

    const FVector Movement =
        (North * Input.X + East * Input.Y) *
        CurrentPanSpeed *
        GetWorld()->GetDeltaSeconds();

    P->AddActorWorldOffset(Movement, true);
}

void UUnifiedMovementComponent::HandleRtsZoom(float Value)
{
    AUnifiedPawn *UnifiedPawn = GetUnifiedPawn();
    if (!UnifiedPawn || FMath::IsNearlyZero(Value))
    {
        return;
    }

    float TargetArmLength = UnifiedPawn->GetTargetArmLength();

    const float ZoomFactor = FMath::Pow(0.9f, Value);
    TargetArmLength *= ZoomFactor;
    UnifiedPawn->SetTargetArmLength(
        FMath::Clamp(TargetArmLength,
                     UnifiedPawn->GetMinZoom(),
                     UnifiedPawn->GetMaxZoom()));
}

void UUnifiedMovementComponent::HandleFreeFlyMove(const FVector2D &Input)
{
    APawn *P = Cast<APawn>(GetOwner());
    if (!P)
    {
        return;
    }

    AController *Controller = P->GetController();
    if (!Controller)
    {
        return;
    }

    const FRotator ControlRot = Controller->GetControlRotation();
    const FVector Forward = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::X);
    const FVector Right = FRotationMatrix(ControlRot).GetUnitAxis(EAxis::Y);

    const FVector Movement =
        (Forward * Input.X + Right * Input.Y) *
        FreeFlyMoveSpeed *
        GetWorld()->GetDeltaSeconds();

    P->AddActorWorldOffset(Movement, true);
}

void UUnifiedMovementComponent::HandleFreeFlyLook(const FVector2D &Input)
{
    APawn *P = Cast<APawn>(GetOwner());
    if (!P)
    {
        return;
    }

    P->AddControllerYawInput(Input.X);
    P->AddControllerPitchInput(Input.Y);
}

void UUnifiedMovementComponent::HandleVerticalMoveInput(float Value)
{
    if (CurrentMode == ECameraMode::FreeFly)
    {
        HandleFreeFlyVerticalMove(Value);
    }
}

void UUnifiedMovementComponent::HandleFreeFlyVerticalMove(float Value)
{
    APawn *P = Cast<APawn>(GetOwner());
    AUnifiedPawn *UnifiedPawn = Cast<AUnifiedPawn>(P);
    if (!P || !UnifiedPawn || FMath::IsNearlyZero(Value))
    {
        return;
    }

    FVector GlobeUp = FVector::UpVector;
    UnifiedPawn->GetGlobeUpVector(GlobeUp);

    const FVector Movement =
        GlobeUp * Value * FreeFlyMoveSpeed * GetWorld()->GetDeltaSeconds();

    P->AddActorWorldOffset(Movement, true);
}

AUnifiedPawn *UUnifiedMovementComponent::GetUnifiedPawn() const
{
    return Cast<AUnifiedPawn>(GetOwner());
}