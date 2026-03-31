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
 * @brief Pawn that supports both RTS and FreeFly camera modes with globe-aware alignment.
 *
 * The pawn uses a USpringArmComponent + UCameraComponent pair for both modes:
 *  - RTS:     The arm has a fixed top-down rotation and a variable length used for zoom.
 *             The pawn body is continuously aligned to the globe surface so that panning
 *             always follows true North/East directions.
 *  - FreeFly: The arm length is collapsed to zero so the camera attaches directly.
 *             The pawn body stays upright relative to the globe, and the controller
 *             rotation drives camera look.
 *
 * Input is forwarded from AUnifiedController to the Handle*() methods here, which
 * delegate to UUnifiedMovementComponent.
 *
 * Globe orientation is derived via CesiumGeoreference transforms so the pawn behaves
 * correctly anywhere on the planet surface.
 */
UCLASS()
class DT4MOB_API AUnifiedPawn : public APawn
{
    GENERATED_BODY()

public:
    /** @brief Constructs the pawn, creates SpringArm, Camera, and MovementComponent sub-objects. */
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
     * @param Value Scalar action value; positive = zoom in.
     */
    void HandleZoom(const FInputActionValue &Value);

    /**
     * @brief Reserved for left-click input handling (currently a no-op on the pawn side).
     * @param Value Input action value (unused).
     */
    void HandleLeftClick(const FInputActionValue &Value);

    /**
     * @brief Forwards scalar vertical-move input to UUnifiedMovementComponent.
     * @param Value Scalar action value; positive = move up.
     */
    void HandleVerticalMove(const FInputActionValue &Value);

    /**
     * @brief Switches the active camera mode and applies mode-specific configuration.
     *
     * Saves the current RTS arm length before switching away from RTS so it can be
     * restored on return. Also notifies the controller to update its input mode.
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
     * Uses CesiumGeoreference to convert the pawn's ECEF position to a surface normal.
     *
     * @param OutUp  Receives the globe-up direction in Unreal world space.
     * @return True if the vector was successfully computed; false if the georeference is missing.
     */
    bool GetGlobeUpVector(FVector &OutUp) const;

    /**
     * @brief Computes the projected North direction in the local tangent plane at the pawn's position.
     *
     * Projects the ECEF North axis (0,0,1) onto the globe tangent plane defined by the globe-up vector.
     *
     * @param OutNorth  Receives the tangent-plane North direction in Unreal world space.
     * @return True if the vector was successfully computed; false if the georeference or globe-up is unavailable.
     */
    bool GetGlobeNorthVector(FVector &OutNorth) const;

    /** @brief Spring arm that positions the camera relative to the pawn root. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    USpringArmComponent *SpringArm;

    /** @brief Returns the pawn's movement component cast to UPawnMovementComponent. */
    virtual UPawnMovementComponent *GetMovementComponent() const override;

    /**
     * @brief Returns the target spring-arm length used by the zoom system.
     * @return Target arm length in Unreal units.
     */
    float GetTargetArmLength() const { return TargetArmLength; }

    /**
     * @brief Sets the target spring-arm length (the actual arm interpolates toward this each tick).
     * @param NewLength New target length in Unreal units.
     */
    void SetTargetArmLength(float NewLength) { TargetArmLength = NewLength; }

    /**
     * @brief Returns the minimum allowed zoom (arm length) in RTS mode.
     * @return Minimum arm length in Unreal units.
     */
    float GetMinZoom() const { return MinZoom; }

    /**
     * @brief Returns the maximum allowed zoom (arm length) in RTS mode.
     * @return Maximum arm length in Unreal units.
     */
    float GetMaxZoom() const { return MaxZoom; }

protected:
    /** @brief Called when play begins; applies the initial camera mode and synchronises controller input mode. */
    virtual void BeginPlay() override;

    /**
     * @brief Called every frame; dispatches per-mode alignment and hover updates.
     * @param DeltaTime Time elapsed since the last frame.
     */
    virtual void Tick(float DeltaTime) override;

private:
    /** @brief Perspective camera attached to the end of the spring arm. */
    UPROPERTY(VisibleAnywhere)
    UCameraComponent *Camera;

    /** @brief Custom movement component that handles all locomotion logic. */
    UPROPERTY(VisibleAnywhere)
    UUnifiedMovementComponent *MovementComponent;

    /** @brief Currently active camera mode. */
    ECameraMode CurrentMode = ECameraMode::RTS;

    /** @brief Applies the current camera mode settings to the spring arm and camera. */
    void ApplyCameraMode();

    /**
     * @brief Per-tick RTS alignment: rotates the pawn to stay aligned with globe North/East.
     * @param DeltaTime Time elapsed since the last frame.
     */
    void UpdateRtsAlignment(float DeltaTime);

    /**
     * @brief Per-tick FreeFly alignment: keeps the controller and pawn upright relative to the globe.
     * @param DeltaTime Time elapsed since the last frame.
     */
    void UpdateFreeFlyAlignment(float DeltaTime);

    /** @brief Instantly snaps the pawn and controller rotation to globe-aligned orientation on FreeFly entry. */
    void SnapFreeFlyToGlobeUp();

    /**
     * @brief Builds a globe-aligned quaternion from the given controller rotation, preserving pitch.
     *
     * @param SourceRotation  The current controller rotation to re-align.
     * @param OutRotation     Receives the aligned quaternion.
     * @return True on success; false if the globe-up vector is unavailable.
     */
    bool BuildFreeFlyGlobeAlignedRotation(const FRotator &SourceRotation, FQuat &OutRotation) const;

    /**
     * @brief Builds an upright pawn rotation whose forward vector matches DesiredForward projected onto the tangent plane.
     *
     * @param DesiredForward  World-space forward direction to align to (will be projected).
     * @param OutRotation     Receives the resulting upright quaternion.
     * @return True on success; false if the globe-up vector or projection is degenerate.
     */
    bool BuildUprightPawnRotationFromForward(const FVector &DesiredForward, FQuat &OutRotation) const;

    /**
     * @brief Performs a line trace downward along the globe-up vector to find the ground surface.
     * @param OutHit  Receives the first blocking hit result.
     * @return True if a ground surface was found within GroundTraceDistance.
     */
    bool TraceGround(FHitResult &OutHit) const;

    /**
     * @brief Adjusts the pawn's height in RTS mode to hover above the ground at a zoom-dependent distance.
     * @param DeltaTime Time elapsed since the last frame.
     */
    void UpdateRtsHover(float DeltaTime);

    /** @brief Target hover height above the ground surface in RTS mode (Unreal units). */
    UPROPERTY(EditAnywhere, Category = "RTS|Hover")
    float HoverHeight = 1000.0f;

    /** @brief Interpolation speed for the hover height correction. */
    UPROPERTY(EditAnywhere, Category = "RTS|Hover")
    float HoverInterpSpeed = 5.0f;

    /** @brief Maximum downward trace distance when searching for the ground. */
    UPROPERTY(EditAnywhere, Category = "RTS|Hover")
    float GroundTraceDistance = 100000000.0f;

    /** @brief Current desired spring-arm length; the actual arm interpolates toward this. */
    float TargetArmLength = 0.f;

    /** @brief Saved spring-arm length from the last time RTS mode was active; restored on re-entry. */
    float RTSArmLength = 1500.f;

    /** @brief Minimum spring-arm length (closest zoom) in RTS mode. */
    float MinZoom = 1500.f;

    /** @brief Maximum spring-arm length (furthest zoom) in RTS mode. */
    float MaxZoom = 750000000.f;

    /** @brief Speed at which the pawn rotation is interpolated toward the globe-aligned target. */
    float RotationInterpSpeed = 5.f;

    /** @brief Arm length set at the start of the game; used as the initial RTS zoom level. */
    float StartingArmLength = 1500.f;

    /** @brief Fixed relative rotation of the spring arm in RTS mode (top-down perspective). */
    FRotator RtsSpringArmRotation = FRotator(-90.f, -90.f, 0.f);
};
