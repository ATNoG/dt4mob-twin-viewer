#pragma once

#include "CoreMinimal.h"
#include "BaseMovementComponent.h"
#include "CameraMode.h"
#include "UnifiedMovementComponent.generated.h"

class AUnifiedPawn;

/**
 * @brief Movement component that implements both RTS and FreeFly camera movement for AUnifiedPawn.
 *
 * Dispatches input to the appropriate private handler based on the active ECameraMode.
 *
 * RTS mode:
 *  - Pan:  Translates the pawn along the globe tangent plane using North/East vectors.
 *          Pan speed scales linearly with the current spring-arm length.
 *  - Zoom: Adjusts the spring-arm target length using an exponential zoom factor,
 *          clamped to [MinZoom, MaxZoom]. The arm length is smoothly interpolated
 *          each tick via ZoomInterpSpeed.
 *
 * FreeFly mode:
 *  - Move: Translates the pawn along the controller's forward/right vectors.
 *  - Look: Adds yaw/pitch input to the controller rotation.
 *  - Vertical: Moves the pawn along the globe-up vector.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DT4MOB_API UUnifiedMovementComponent : public UBaseMovementComponent
{
    GENERATED_BODY()

public:
    /** @brief Enables ticking so the spring-arm zoom interpolation is applied each frame. */
    UUnifiedMovementComponent();

    /**
     * @brief Sets the active camera mode so subsequent inputs are routed correctly.
     * @param NewMode The camera mode to activate.
     */
    void SetCameraMode(ECameraMode NewMode);

    /**
     * @brief Returns the currently active camera mode.
     * @return The ECameraMode value currently in effect.
     */
    ECameraMode GetCameraMode() const { return CurrentMode; }

    /**
     * @brief Dispatches 2D movement input to either HandleRtsMove or HandleFreeFlyMove.
     * @param Input Normalised 2D input (X = forward/north, Y = right/east).
     */
    virtual void HandleMoveInput(const FVector2D &Input) override;

    /**
     * @brief Dispatches look input; only active in FreeFly mode.
     * @param Input 2D input (X = yaw delta, Y = pitch delta).
     */
    virtual void HandleLookInput(const FVector2D &Input) override;

    /**
     * @brief Dispatches zoom input; only active in RTS mode.
     * @param Value Zoom delta (positive = zoom in, negative = zoom out).
     */
    virtual void HandleZoomInput(float Value) override;

    /**
     * @brief Handles vertical (globe-up) movement input; only active in FreeFly mode.
     * @param Value Signed scalar — positive moves up along the globe-up vector.
     */
    void HandleVerticalMoveInput(float Value);

protected:
    /** @brief Called when the component is initialised. */
    virtual void BeginPlay() override;

    /**
     * @brief Each tick, smoothly interpolates the spring-arm length toward the target (RTS mode only).
     * @param DeltaTime       Time since last tick.
     * @param TickType        Type of tick (actor, component, etc.).
     * @param ThisTickFunction Tick function structure.
     */
    virtual void TickComponent(
        float DeltaTime,
        enum ELevelTick TickType,
        FActorComponentTickFunction *ThisTickFunction) override;

private:
    /**
     * @brief Pans the pawn along the globe tangent plane in RTS mode.
     * @param Input 2D input (X = north, Y = east).
     */
    void HandleRtsMove(const FVector2D &Input);

    /**
     * @brief Adjusts the spring-arm target length in RTS mode using an exponential zoom factor.
     * @param Value Mouse-wheel delta; positive = zoom in.
     */
    void HandleRtsZoom(float Value);

    /**
     * @brief Moves the pawn along the controller's forward/right axes in FreeFly mode.
     * @param Input 2D input (X = forward, Y = right).
     */
    void HandleFreeFlyMove(const FVector2D &Input);

    /**
     * @brief Applies yaw and pitch to the controller rotation in FreeFly mode.
     * @param Input 2D input (X = yaw, Y = pitch).
     */
    void HandleFreeFlyLook(const FVector2D &Input);

    /**
     * @brief Moves the pawn along the globe-up direction in FreeFly mode.
     * @param Value Signed scalar velocity multiplier.
     */
    void HandleFreeFlyVerticalMove(float Value);

    /**
     * @brief Convenience helper that casts the component's owner to AUnifiedPawn.
     * @return Cast result, or nullptr if the owner is not an AUnifiedPawn.
     */
    AUnifiedPawn *GetUnifiedPawn() const;

    /** @brief Active camera mode; determines which input handler is invoked. */
    UPROPERTY(EditAnywhere, Category = "Mode")
    ECameraMode CurrentMode = ECameraMode::FreeFly;

    /** @brief Base pan speed multiplier for RTS mode (scaled by arm length at runtime). */
    UPROPERTY(EditAnywhere, Category = "RTS")
    float BasePanSpeed = 1.0f;

    /** @brief Zoom speed for RTS mode (raw scroll delta scale before the exponential factor). */
    UPROPERTY(EditAnywhere, Category = "RTS")
    float ZoomSpeed = 5000.f;

    /** @brief Interpolation speed used to smooth the spring-arm length each tick. */
    UPROPERTY(EditAnywhere, Category = "RTS")
    float ZoomInterpSpeed = 5.f;

    /** @brief Translation speed in Unreal units per second in FreeFly mode. */
    UPROPERTY(EditAnywhere, Category = "FreeFly")
    float FreeFlyMoveSpeed = 3000.f;
};
