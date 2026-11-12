/**
 * @file
 * @brief Declares UUnifiedMovementComponent, which moves AUnifiedPawn in RTS and FreeFly modes.
 * @ingroup gameplay
 */

#pragma once

#include "CoreMinimal.h"
#include "BaseMovementComponent.h"
#include "CameraMode.h"
#include "UnifiedMovementComponent.generated.h"

class AUnifiedPawn;

/**
 * @brief Movement component that implements both RTS and FreeFly camera movement for AUnifiedPawn.
 *
 * Each input handler checks the active ECameraMode and forwards to the matching private
 * handler. Translations use AddActorWorldOffset() with sweeping enabled and are scaled by the
 * world delta time, because the handlers run once per input event rather than from Tick.
 *
 * RTS mode:
 *  - Pan:  Translates the pawn along the globe tangent plane using the pawn's North/East
 *          vectors. Speed is BasePanSpeed times the current spring-arm length, so panning
 *          covers more ground when zoomed out.
 *  - Zoom: Multiplies the pawn's target arm length by 0.9 per unit of input and clamps it to
 *          the pawn's MinZoom/MaxZoom. TickComponent() interpolates the real arm toward it at
 *          ZoomInterpSpeed.
 *
 * FreeFly mode:
 *  - Move: Translates the pawn along the control rotation's forward/right axes (forward
 *          includes pitch, so the pawn flies where the camera looks).
 *  - Look: Adds yaw/pitch input to the controller rotation.
 *  - Vertical: Moves the pawn along the globe-up vector.
 *
 * @ingroup gameplay
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DT4MOB_API UUnifiedMovementComponent : public UBaseMovementComponent
{
    GENERATED_BODY()

public:
    /** @brief Enables ticking so the spring-arm zoom interpolation is applied each frame. */
    UUnifiedMovementComponent();

    /**
     * @brief Sets the mode that decides how subsequent input is handled.
     *
     * Called by AUnifiedPawn whenever it applies a camera mode; it does not touch the pawn or
     * spring arm itself.
     *
     * @param NewMode The camera mode to activate.
     */
    void SetCameraMode(ECameraMode NewMode);

    /**
     * @brief Returns the mode this component is currently handling input for.
     * @return The ECameraMode value currently in effect.
     */
    ECameraMode GetCameraMode() const { return CurrentMode; }

    /**
     * @brief Pans along the globe in RTS mode, or flies along the view axes in FreeFly mode.
     * @param Input Normalised 2D input (X = forward/north, Y = right/east).
     */
    virtual void HandleMoveInput(const FVector2D &Input) override;

    /**
     * @brief Applies look input in FreeFly mode; ignored in RTS mode.
     * @param Input 2D input (X = yaw delta, Y = pitch delta).
     */
    virtual void HandleLookInput(const FVector2D &Input) override;

    /**
     * @brief Applies zoom input in RTS mode; ignored in FreeFly mode.
     * @param Value Zoom delta (positive zooms in, negative zooms out).
     */
    virtual void HandleZoomInput(float Value) override;

    /**
     * @brief Applies vertical (globe-up) movement input in FreeFly mode; ignored in RTS mode.
     * @param Value Signed scalar; positive moves up along the globe-up vector.
     */
    void HandleVerticalMoveInput(float Value);

protected:
    /** @brief Calls the base implementation only; this component needs no extra setup. */
    virtual void BeginPlay() override;

    /**
     * @brief In RTS mode, interpolates the spring arm's TargetArmLength toward the pawn's target zoom.
     *
     * Uses FMath::FInterpTo() with ZoomInterpSpeed. Does nothing in FreeFly mode or when the
     * owner is not an AUnifiedPawn with a spring arm.
     *
     * @param DeltaTime        Time since the last tick, in seconds.
     * @param TickType         Kind of tick being run.
     * @param ThisTickFunction Tick function that is running this tick.
     */
    virtual void TickComponent(
        float DeltaTime,
        enum ELevelTick TickType,
        FActorComponentTickFunction *ThisTickFunction) override;

private:
    /**
     * @brief Pans the pawn along the globe tangent plane in RTS mode.
     *
     * Does nothing if the globe up or North vector cannot be computed (no georeference).
     *
     * @param Input 2D input (X = north, Y = east).
     */
    void HandleRtsMove(const FVector2D &Input);

    /**
     * @brief Scales the pawn's target arm length by 0.9 raised to Value and clamps it to the zoom limits.
     * @param Value Mouse-wheel delta; positive zooms in. Values near zero are ignored.
     */
    void HandleRtsZoom(float Value);

    /**
     * @brief Moves the pawn along the control rotation's forward/right axes in FreeFly mode.
     *
     * Does nothing if the pawn has no controller.
     *
     * @param Input 2D input (X = forward, Y = right).
     */
    void HandleFreeFlyMove(const FVector2D &Input);

    /**
     * @brief Adds yaw and pitch input to the owning pawn's controller in FreeFly mode.
     * @param Input 2D input (X = yaw, Y = pitch).
     */
    void HandleFreeFlyLook(const FVector2D &Input);

    /**
     * @brief Moves the pawn along the globe-up direction in FreeFly mode.
     *
     * Falls back to world +Z if the globe-up vector cannot be computed.
     *
     * @param Value Signed speed multiplier applied to FreeFlyMoveSpeed. Values near zero are ignored.
     */
    void HandleFreeFlyVerticalMove(float Value);

    /**
     * @brief Casts the component's owner to AUnifiedPawn.
     * @return The owning pawn, or nullptr if the owner is not an AUnifiedPawn.
     */
    AUnifiedPawn *GetUnifiedPawn() const;

    /**
     * @brief Mode that decides which input handler runs.
     *
     * The FreeFly default is overwritten by AUnifiedPawn::BeginPlay(), which pushes the pawn's
     * own mode through SetCameraMode().
     */
    UPROPERTY(EditAnywhere, Category = "Mode")
    ECameraMode CurrentMode = ECameraMode::FreeFly;

    /** @brief RTS pan speed as a multiple of the spring-arm length: at 1.0 and full input the pawn moves one arm length per second. */
    UPROPERTY(EditAnywhere, Category = "RTS")
    float BasePanSpeed = 1.0f;

    /** @brief Intended RTS zoom speed. Not read by the current code; HandleRtsZoom() uses a fixed 0.9 factor per input unit. */
    UPROPERTY(EditAnywhere, Category = "RTS")
    float ZoomSpeed = 5000.f;

    /** @brief FInterpTo() speed used to move the spring-arm length toward the target zoom each tick. */
    UPROPERTY(EditAnywhere, Category = "RTS")
    float ZoomInterpSpeed = 5.f;

    /** @brief FreeFly translation speed in Unreal units (cm) per second at full input, horizontal and vertical. */
    UPROPERTY(EditAnywhere, Category = "FreeFly")
    float FreeFlyMoveSpeed = 3000.f;
};
