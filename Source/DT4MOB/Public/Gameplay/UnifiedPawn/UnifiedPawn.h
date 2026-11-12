/**
 * @file
 * @brief Declares AUnifiedPawn, the globe-aware camera pawn with RTS and FreeFly modes.
 * @ingroup gameplay
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "CameraMode.h"
#include "InputActionValue.h"
#include "GameFramework/SpringArmComponent.h"
#include "UnifiedPawn.generated.h"

class UCameraComponent;
class UUnifiedMovementComponent;
class UPawnMovementComponent;

/**
 * @brief Camera pawn that supports RTS and FreeFly modes and keeps itself aligned to the globe.
 *
 * The pawn uses a USpringArmComponent + UCameraComponent pair for both modes:
 *  - RTS:     The arm keeps the fixed relative rotation RtsSpringArmRotation (looking straight
 *             down) and its length is the zoom distance. Every tick the pawn body is rotated
 *             toward a frame built from the local East, North and up vectors so panning follows
 *             the globe, and the pawn is kept a zoom-dependent height above the ground.
 *  - FreeFly: The arm length is set to zero and the arm follows the control rotation. Every
 *             tick the control rotation is levelled against the globe (roll removed, pitch kept)
 *             and the pawn body is kept upright with the camera's heading.
 *
 * AUnifiedController forwards input to the Handle*() methods, which pass it on to
 * UUnifiedMovementComponent.
 *
 * Globe directions come from the default ACesiumGeoreference, so the pawn behaves the same
 * anywhere on the planet.
 *
 * @ingroup gameplay
 */
UCLASS()
class DT4MOB_API AUnifiedPawn : public APawn
{
    GENERATED_BODY()

public:
    /** @brief Creates the root, SpringArm, Camera and MovementComponent sub-objects and enables ticking. */
    AUnifiedPawn();

    /**
     * @brief Forwards 2D move input to UUnifiedMovementComponent.
     * @param Value FVector2D action value (X = north/forward, Y = east/right).
     */
    void HandleMove(const FInputActionValue &Value);

    /**
     * @brief Forwards 2D look input to UUnifiedMovementComponent.
     * @param Value FVector2D action value (X = yaw, Y = pitch).
     */
    void HandleLook(const FInputActionValue &Value);

    /**
     * @brief Forwards scalar zoom input to UUnifiedMovementComponent.
     * @param Value Scalar action value; positive zooms in.
     */
    void HandleZoom(const FInputActionValue &Value);

    /**
     * @brief Empty placeholder for left-click input.
     *
     * AUnifiedController handles clicks itself and does not call this.
     *
     * @param Value Input action value (not read).
     */
    void HandleLeftClick(const FInputActionValue &Value);

    /**
     * @brief Forwards scalar vertical-move input to UUnifiedMovementComponent.
     * @param Value Scalar action value; positive moves up.
     */
    void HandleVerticalMove(const FInputActionValue &Value);

    /**
     * @brief Switches the active camera mode and applies the mode-specific setup.
     *
     * Does nothing if NewMode is already active. When leaving RTS, the current target arm
     * length is saved in RTSArmLength so it can be restored on return. After ApplyCameraMode()
     * it calls AUnifiedController::ApplyGameInputMode() if the pawn is controlled by one.
     *
     * @param NewMode The camera mode to activate.
     */
    void SetCameraMode(ECameraMode NewMode);

    /**
     * @brief Returns the currently active camera mode.
     * @return The ECameraMode in effect.
     */
    ECameraMode GetCameraMode() const { return CurrentMode; }

    /**
     * @brief Computes the globe-up unit vector at the pawn's current world position.
     *
     * The direction is the pawn's ECEF position normalised (the geocentric radial direction,
     * not the ellipsoid surface normal), converted to Unreal world space through the default
     * CesiumGeoreference.
     *
     * @param OutUp  Receives the globe-up direction in Unreal world space.
     * @return False if there is no georeference, the pawn sits at the Earth's centre, or the
     *         converted vector is degenerate; true otherwise.
     */
    bool GetGlobeUpVector(FVector &OutUp) const;

    /**
     * @brief Computes the North direction in the local tangent plane at the pawn's position.
     *
     * Projects the ECEF North axis (0,0,1) onto the tangent plane defined by the globe-up vector.
     *
     * @param OutNorth  Receives the tangent-plane North direction in Unreal world space.
     * @return False if the georeference or globe-up vector is unavailable, or the projection is
     *         degenerate (at the poles); true otherwise.
     */
    bool GetGlobeNorthVector(FVector &OutNorth) const;

    /**
     * @brief Spring arm that holds the camera.
     *
     * In RTS mode its TargetArmLength is the zoom distance, interpolated toward
     * GetTargetArmLength() by UUnifiedMovementComponent. In FreeFly mode its length is 0.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USpringArmComponent *SpringArm;

    /**
     * @brief Returns the pawn's movement component.
     * @return The UUnifiedMovementComponent created in the constructor, as a UPawnMovementComponent.
     */
    virtual UPawnMovementComponent *GetMovementComponent() const override;

    /**
     * @brief Returns the target spring-arm length used by the RTS zoom.
     *
     * Entering FreeFly collapses the spring arm but leaves this value alone, so it still holds
     * the last RTS zoom while in FreeFly. ADT4MOBGamemode adds it to the pawn height to
     * estimate the camera altitude.
     *
     * @return Target arm length in Unreal units (cm).
     */
    float GetTargetArmLength() const { return TargetArmLength; }

    /**
     * @brief Sets the target spring-arm length; the real arm interpolates toward it each tick in RTS mode.
     *
     * The value is not clamped here; UUnifiedMovementComponent clamps it to GetMinZoom() and
     * GetMaxZoom() before calling this.
     *
     * @param NewLength New target length in Unreal units (cm).
     */
    void SetTargetArmLength(float NewLength) { TargetArmLength = NewLength; }

    /**
     * @brief Returns the closest allowed RTS zoom.
     * @return Minimum arm length in Unreal units (cm).
     */
    float GetMinZoom() const { return MinZoom; }

    /**
     * @brief Returns the farthest allowed RTS zoom.
     * @return Maximum arm length in Unreal units (cm).
     */
    float GetMaxZoom() const { return MaxZoom; }

protected:
    /** @brief Applies the initial camera mode and syncs the controller's input mode to it. */
    virtual void BeginPlay() override;

    /**
     * @brief Runs the per-mode updates: RTS alignment and ground hover in RTS, globe alignment in FreeFly.
     * @param DeltaTime Time elapsed since the last frame, in seconds.
     */
    virtual void Tick(float DeltaTime) override;

private:
    /** @brief Perspective camera attached to the spring arm socket. It does not follow the control rotation itself; in FreeFly the arm does. */
    UPROPERTY(VisibleAnywhere)
    UCameraComponent *Camera;

    /** @brief Movement component that turns forwarded input into pawn motion and zoom. */
    UPROPERTY(VisibleAnywhere)
    UUnifiedMovementComponent *MovementComponent;

    /** @brief Currently active camera mode. Change it through SetCameraMode(). */
    ECameraMode CurrentMode = ECameraMode::RTS;

    /**
     * @brief Applies CurrentMode to the movement component, spring arm and camera.
     *
     * RTS restores RTSArmLength as both target and actual arm length, turns off control
     * rotation on the pawn, arm and camera, and restores RtsSpringArmRotation. FreeFly sets the
     * arm length to 0, makes the arm follow the control rotation, zeroes its relative rotation
     * and calls SnapFreeFlyToGlobeUp().
     */
    void ApplyCameraMode();

    /**
     * @brief Per-tick RTS alignment: slerps the pawn toward a globe-aligned frame and re-applies the arm rotation.
     *
     * The target frame has X = East, Y = North and Z = globe up. The slerp factor is
     * DeltaTime * 3. Does nothing if the globe vectors cannot be computed.
     *
     * @param DeltaTime Time elapsed since the last frame, in seconds.
     */
    void UpdateRtsAlignment(float DeltaTime);

    /**
     * @brief Per-tick FreeFly alignment: keeps the control rotation level against the globe and the pawn upright.
     *
     * The control rotation is slerped toward BuildFreeFlyGlobeAlignedRotation() with factor
     * DeltaTime * 3, then the pawn body toward an upright rotation with the same heading with
     * factor DeltaTime * 5. Does nothing without a player controller.
     *
     * @param DeltaTime Time elapsed since the last frame, in seconds.
     */
    void UpdateFreeFlyAlignment(float DeltaTime);

    /** @brief Snaps the control rotation and pawn rotation to the globe-aligned FreeFly orientation without smoothing. Used on FreeFly entry. */
    void SnapFreeFlyToGlobeUp();

    /**
     * @brief Builds a globe-aligned control rotation from the given rotation, keeping heading and pitch.
     *
     * Removes roll relative to the local globe up. The angle between the view direction and
     * globe up is preserved, and the horizontal heading comes from the source right vector
     * projected onto the tangent plane (with fallbacks when that projection degenerates).
     *
     * @param SourceRotation  Current control rotation to re-align.
     * @param OutRotation     Receives the aligned quaternion.
     * @return True on success; false if the globe-up vector is unavailable or no stable horizontal axis can be found.
     */
    bool BuildFreeFlyGlobeAlignedRotation(const FRotator &SourceRotation, FQuat &OutRotation) const;

    /**
     * @brief Builds an upright pawn rotation whose forward vector is DesiredForward projected onto the tangent plane.
     *
     * If DesiredForward points straight up or down, the actor's forward vector and then world
     * +X are tried instead.
     *
     * @param DesiredForward  World-space forward direction to align to (projected before use).
     * @param OutRotation     Receives the resulting upright quaternion (Z = globe up).
     * @return True on success; false if the globe-up vector is unavailable or every projection is degenerate.
     */
    bool BuildUprightPawnRotationFromForward(const FVector &DesiredForward, FQuat &OutRotation) const;

    /**
     * @brief Line-traces along the globe-up axis on the Visibility channel to find the ground below the pawn.
     *
     * The trace starts GroundTraceDistance / 2 above the pawn and ends GroundTraceDistance below
     * it, ignoring the pawn itself.
     *
     * @param OutHit  Receives the first blocking hit.
     * @return True if ground was hit; false if nothing was hit or the globe-up vector is unavailable.
     */
    bool TraceGround(FHitResult &OutHit) const;

    /**
     * @brief Moves the pawn toward a zoom-dependent height above the ground in RTS mode.
     *
     * The height is the current spring-arm length mapped linearly from MinZoom..MaxZoom onto
     * 200..50000 cm (clamped), measured along globe up from the TraceGround() hit. The pawn
     * position is interpolated with HoverInterpSpeed. Does nothing if no ground is found.
     *
     * @param DeltaTime Time elapsed since the last frame, in seconds.
     */
    void UpdateRtsHover(float DeltaTime);

    /** @brief Intended RTS hover height above the ground (cm). Not read by the current code; UpdateRtsHover() derives the height from the zoom. */
    UPROPERTY(EditAnywhere, Category = "RTS|Hover")
    float HoverHeight = 1000.0f;

    /** @brief VInterpTo() speed used by UpdateRtsHover() to move the pawn toward its hover height. */
    UPROPERTY(EditAnywhere, Category = "RTS|Hover")
    float HoverInterpSpeed = 5.0f;

    /** @brief Length of the downward part of the ground trace, in cm (default 1,000 km). The trace also starts half this distance above the pawn. */
    UPROPERTY(EditAnywhere, Category = "RTS|Hover")
    float GroundTraceDistance = 100000000.0f;

    /** @brief Target spring-arm length (cm) for RTS zoom; the real arm interpolates toward it. */
    float TargetArmLength = 0.f;

    /** @brief Target arm length saved when leaving RTS mode and restored on re-entry (cm). */
    float RTSArmLength = 1500.f;

    /** @brief Minimum spring-arm length (closest zoom) in RTS mode, in cm (15 m). */
    float MinZoom = 1500.f;

    /** @brief Maximum spring-arm length (farthest zoom) in RTS mode, in cm (7,500 km). */
    float MaxZoom = 750000000.f;

    /** @brief Intended rotation smoothing speed. Not read by the current code; the alignment functions use fixed slerp factors. */
    float RotationInterpSpeed = 5.f;

    /** @brief Spring-arm length (cm) applied in the constructor; also the initial RTSArmLength. */
    float StartingArmLength = 1500.f;

    /** @brief Relative rotation of the spring arm in RTS mode (pitch -90, yaw -90), giving a straight-down view. */
    FRotator RtsSpringArmRotation = FRotator(-90.f, -90.f, 0.f);
};
