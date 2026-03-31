/** @file UnifiedPawn.cpp
 *  @brief Implementation of AUnifiedPawn. All logic documentation is in the header.
 */
#include "Gameplay/UnifiedPawn/UnifiedPawn.h"

#include "Camera/CameraComponent.h"
#include "Gameplay/UnifiedPawn/UnifiedMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Gameplay/UnifiedPawn/UnifiedController.h"
#include "GameFramework/PlayerController.h"
#include "CesiumGeoreference.h"
#include "DrawDebugHelpers.h"

AUnifiedPawn::AUnifiedPawn()
{
    PrimaryActorTick.bCanEverTick = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = StartingArmLength;
    SpringArm->bDoCollisionTest = false;
    SpringArm->SetRelativeRotation(RtsSpringArmRotation);

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);

    TargetArmLength = SpringArm->TargetArmLength;

    // Separate per-mode
    RTSArmLength = StartingArmLength;

    MovementComponent = CreateDefaultSubobject<UUnifiedMovementComponent>(TEXT("MovementComponent"));
}

void AUnifiedPawn::BeginPlay()
{
    Super::BeginPlay();
    ApplyCameraMode();

    // Ensure the controller's input mode is in sync with the pawn's camera mode at start
    if (AUnifiedController *UnifiedCtrl = Cast<AUnifiedController>(GetController()))
    {
        UnifiedCtrl->ApplyGameInputMode(CurrentMode);
    }
}

void AUnifiedPawn::SetCameraMode(ECameraMode NewMode)
{
    if (CurrentMode == NewMode)
    {
        return;
    }

    if (CurrentMode == ECameraMode::RTS)
    {
        RTSArmLength = TargetArmLength;
    }

    CurrentMode = NewMode;
    ApplyCameraMode();
    if (AUnifiedController *UnifiedCtrl = Cast<AUnifiedController>(GetController()))
    {
        UnifiedCtrl->ApplyGameInputMode(NewMode);
    }
}

void AUnifiedPawn::ApplyCameraMode()
{
    if (MovementComponent)
    {
        MovementComponent->SetCameraMode(CurrentMode);
    }

    if (CurrentMode == ECameraMode::RTS)
    {
        TargetArmLength = RTSArmLength;
        if (SpringArm)
        {
            SpringArm->TargetArmLength = RTSArmLength;
        }

        bUseControllerRotationPitch = false;
        bUseControllerRotationYaw = false;
        bUseControllerRotationRoll = false;
        SpringArm->bUsePawnControlRotation = false;
        Camera->bUsePawnControlRotation = false;
        SpringArm->SetRelativeRotation(RtsSpringArmRotation);
    }
    else if (CurrentMode == ECameraMode::FreeFly)
    {
        if (SpringArm)
        {
            SpringArm->TargetArmLength = 0.f;
        }

        bUseControllerRotationPitch = false;
        bUseControllerRotationYaw = false;
        bUseControllerRotationRoll = false;

        SpringArm->bUsePawnControlRotation = true;
        Camera->bUsePawnControlRotation = false;
        SpringArm->SetRelativeRotation(FRotator::ZeroRotator);

        SnapFreeFlyToGlobeUp();
    }
}

void AUnifiedPawn::HandleMove(const FInputActionValue &Value)
{
    if (MovementComponent)
    {
        MovementComponent->HandleMoveInput(Value.Get<FVector2D>());
    }
}

void AUnifiedPawn::HandleLook(const FInputActionValue &Value)
{
    if (MovementComponent)
    {
        MovementComponent->HandleLookInput(Value.Get<FVector2D>());
    }
}

void AUnifiedPawn::HandleZoom(const FInputActionValue &Value)
{
    if (MovementComponent)
    {
        MovementComponent->HandleZoomInput(Value.Get<float>());
    }
}

void AUnifiedPawn::HandleLeftClick(const FInputActionValue &Value)
{
}

void AUnifiedPawn::HandleVerticalMove(const FInputActionValue &Value)
{
    if (MovementComponent)
    {
        MovementComponent->HandleVerticalMoveInput(Value.Get<float>());
    }
}

bool AUnifiedPawn::GetGlobeUpVector(FVector &OutUp) const
{
    ACesiumGeoreference *Georeference =
        ACesiumGeoreference::GetDefaultGeoreferenceForActor(const_cast<AUnifiedPawn *>(this));
    if (!Georeference)
    {
        return false;
    }

    const FVector UnrealPosition = GetActorLocation();
    const FVector EcefPosition =
        Georeference->TransformUnrealPositionToEarthCenteredEarthFixed(UnrealPosition);

    if (EcefPosition.IsNearlyZero())
    {
        return false;
    }

    const FVector EcefUp = EcefPosition.GetSafeNormal();
    OutUp = Georeference->TransformEarthCenteredEarthFixedDirectionToUnreal(EcefUp).GetSafeNormal();
    return !OutUp.IsNearlyZero();
}

void AUnifiedPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (CurrentMode == ECameraMode::RTS)
    {
        UpdateRtsAlignment(DeltaTime);
        UpdateRtsHover(DeltaTime);
    }
    else if (CurrentMode == ECameraMode::FreeFly)
    {
        UpdateFreeFlyAlignment(DeltaTime);
    }
}

void AUnifiedPawn::UpdateRtsAlignment(float DeltaTime)
{
    FVector GlobeUp;
    if (!GetGlobeUpVector(GlobeUp))
    {
        return;
    }

    FVector North;
    if (!GetGlobeNorthVector(North))
    {
        return;
    }

    const FVector East = FVector::CrossProduct(GlobeUp, North).GetSafeNormal();

    // Because of the current spring-arm orientation, screen-up maps better to East/West
    const FVector TangentForward = East;
    const FVector TangentRight = North;

    FMatrix RotMatrix;
    RotMatrix.SetAxis(0, TangentForward);
    RotMatrix.SetAxis(1, TangentRight);
    RotMatrix.SetAxis(2, GlobeUp);

    const FQuat TargetQuat(RotMatrix);
    const FQuat Smoothed = FQuat::Slerp(GetActorQuat(), TargetQuat, DeltaTime * 3.f);

    SetActorRotation(Smoothed.GetNormalized());

    SpringArm->SetRelativeRotation(RtsSpringArmRotation);
}

void AUnifiedPawn::UpdateFreeFlyAlignment(float DeltaTime)
{
    APlayerController *PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return;
    }

    FQuat TargetControlQuat;
    if (!BuildFreeFlyGlobeAlignedRotation(PC->GetControlRotation(), TargetControlQuat))
    {
        return;
    }

    const FQuat CurrentControlQuat = PC->GetControlRotation().Quaternion();
    const FQuat SmoothedControlQuat =
        FQuat::Slerp(CurrentControlQuat, TargetControlQuat, DeltaTime * 3.0f).GetNormalized();

    PC->SetControlRotation(SmoothedControlQuat.Rotator());

    FQuat TargetPawnQuat;
    if (!BuildUprightPawnRotationFromForward(SmoothedControlQuat.GetForwardVector(), TargetPawnQuat))
    {
        return;
    }

    const FQuat SmoothedPawnQuat =
        FQuat::Slerp(GetActorQuat(), TargetPawnQuat, DeltaTime * 5.0f).GetNormalized();

    SetActorRotation(SmoothedPawnQuat);
}

void AUnifiedPawn::SnapFreeFlyToGlobeUp()
{
    APlayerController *PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return;
    }

    FQuat TargetControlQuat;
    if (!BuildFreeFlyGlobeAlignedRotation(PC->GetControlRotation(), TargetControlQuat))
    {
        return;
    }

    PC->SetControlRotation(TargetControlQuat.Rotator());

    FQuat TargetPawnQuat;
    if (BuildUprightPawnRotationFromForward(TargetControlQuat.GetForwardVector(), TargetPawnQuat))
    {
        SetActorRotation(TargetPawnQuat);
    }
}

bool AUnifiedPawn::BuildFreeFlyGlobeAlignedRotation(const FRotator &SourceRotation, FQuat &OutRotation) const
{
    FVector GlobeUp;
    if (!GetGlobeUpVector(GlobeUp))
    {
        return false;
    }

    const FQuat SourceQuat = SourceRotation.Quaternion();

    const FVector Forward = SourceQuat.GetForwardVector().GetSafeNormal();
    const FVector CurrentRight = SourceQuat.GetRightVector().GetSafeNormal();

    if (Forward.IsNearlyZero() || CurrentRight.IsNearlyZero())
    {
        return false;
    }

    // Preserve how much the player is looking up/down relative to the globe
    const float VerticalDot = FVector::DotProduct(Forward, GlobeUp);

    // Build a stable horizontal right vector using the current camera right,
    // projected onto the local tangent plane.
    FVector TangentRight = FVector::VectorPlaneProject(CurrentRight, GlobeUp).GetSafeNormal();

    if (TangentRight.IsNearlyZero())
    {
        TangentRight = FVector::CrossProduct(GlobeUp, Forward).GetSafeNormal();
    }

    if (TangentRight.IsNearlyZero())
    {
        TangentRight = FVector::VectorPlaneProject(GetActorRightVector(), GlobeUp).GetSafeNormal();
    }

    if (TangentRight.IsNearlyZero())
    {
        return false;
    }

    // Horizontal forward direction in the local tangent plane
    FVector TangentForward = FVector::CrossProduct(TangentRight, GlobeUp).GetSafeNormal();
    if (TangentForward.IsNearlyZero())
    {
        return false;
    }

    // Rebuild forward while preserving pitch
    const float HorizontalScale = FMath::Sqrt(FMath::Max(0.f, 1.f - VerticalDot * VerticalDot));
    FVector DesiredForward = (TangentForward * HorizontalScale) + (GlobeUp * VerticalDot);
    DesiredForward = DesiredForward.GetSafeNormal();

    // Rebuild orthonormal basis
    FVector DesiredRight = FVector::CrossProduct(GlobeUp, DesiredForward).GetSafeNormal();
    if (DesiredRight.IsNearlyZero())
    {
        DesiredRight = TangentRight;
    }

    FVector DesiredUp = FVector::CrossProduct(DesiredForward, DesiredRight).GetSafeNormal();
    if (DesiredUp.IsNearlyZero())
    {
        DesiredUp = GlobeUp;
    }

    FMatrix RotMatrix;
    RotMatrix.SetAxis(0, DesiredForward);
    RotMatrix.SetAxis(1, DesiredRight);
    RotMatrix.SetAxis(2, DesiredUp);

    OutRotation = FQuat(RotMatrix).GetNormalized();
    return true;
}

bool AUnifiedPawn::BuildUprightPawnRotationFromForward(const FVector &DesiredForward, FQuat &OutRotation) const
{
    FVector GlobeUp;
    if (!GetGlobeUpVector(GlobeUp))
    {
        return false;
    }

    FVector FlatForward = FVector::VectorPlaneProject(DesiredForward, GlobeUp).GetSafeNormal();
    if (FlatForward.IsNearlyZero())
    {
        FlatForward = FVector::VectorPlaneProject(GetActorForwardVector(), GlobeUp).GetSafeNormal();
    }

    if (FlatForward.IsNearlyZero())
    {
        FlatForward = FVector::VectorPlaneProject(FVector::ForwardVector, GlobeUp).GetSafeNormal();
    }

    if (FlatForward.IsNearlyZero())
    {
        return false;
    }

    const FVector Right = FVector::CrossProduct(GlobeUp, FlatForward).GetSafeNormal();
    FlatForward = FVector::CrossProduct(Right, GlobeUp).GetSafeNormal();

    FMatrix RotMatrix;
    RotMatrix.SetAxis(0, FlatForward);
    RotMatrix.SetAxis(1, Right);
    RotMatrix.SetAxis(2, GlobeUp);

    OutRotation = FQuat(RotMatrix).GetNormalized();
    return true;
}

UPawnMovementComponent *AUnifiedPawn::GetMovementComponent() const
{
    return MovementComponent;
}

bool AUnifiedPawn::TraceGround(FHitResult &OutHit) const
{
    FVector GlobeUp;
    if (!GetGlobeUpVector(GlobeUp))
    {
        return false;
    }

    const FVector Start = GetActorLocation() + GlobeUp * GroundTraceDistance * 0.5f;
    const FVector End = GetActorLocation() - GlobeUp * GroundTraceDistance;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(RTSGroundTrace), false, this);

    return GetWorld()->LineTraceSingleByChannel(
        OutHit,
        Start,
        End,
        ECC_Visibility,
        Params);
}

void AUnifiedPawn::UpdateRtsHover(float DeltaTime)
{
    FHitResult GroundHit;
    if (!TraceGround(GroundHit))
    {
        return;
    }

    FVector GlobeUp;
    if (!GetGlobeUpVector(GlobeUp))
    {
        return;
    }

    const float ZoomRatio = FMath::GetMappedRangeValueClamped(
        FVector2D(MinZoom, MaxZoom),
        FVector2D(200.0f, 50000.0f),
        SpringArm ? SpringArm->TargetArmLength : TargetArmLength);

    const FVector DesiredLocation = GroundHit.Location + GlobeUp * ZoomRatio;

    const FVector NewLocation = FMath::VInterpTo(
        GetActorLocation(),
        DesiredLocation,
        DeltaTime,
        HoverInterpSpeed);

    SetActorLocation(NewLocation);
}

bool AUnifiedPawn::GetGlobeNorthVector(FVector &OutNorth) const
{
    ACesiumGeoreference *Georeference =
        ACesiumGeoreference::GetDefaultGeoreferenceForActor(const_cast<AUnifiedPawn *>(this));
    if (!Georeference)
    {
        return false;
    }

    FVector GlobeUp;
    if (!GetGlobeUpVector(GlobeUp))
    {
        return false;
    }

    // Global north in ECEF
    const FVector EcefNorthAxis(0.0, 0.0, 1.0);

    const FVector UnrealNorthAxis =
        Georeference->TransformEarthCenteredEarthFixedDirectionToUnreal(EcefNorthAxis).GetSafeNormal();

    // Project global north into the local tangent plane
    OutNorth = FVector::VectorPlaneProject(UnrealNorthAxis, GlobeUp).GetSafeNormal();

    return !OutNorth.IsNearlyZero();
}