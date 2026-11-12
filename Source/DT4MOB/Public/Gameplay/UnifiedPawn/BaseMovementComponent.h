/**
 * @file
 * @brief Declares UBaseMovementComponent, the abstract input interface for AUnifiedPawn movement.
 * @ingroup gameplay
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "BaseMovementComponent.generated.h"

/**
 * @brief Abstract base class for pawn movement components used by AUnifiedPawn.
 *
 * Declares the input-handling interface that concrete movement implementations
 * (currently UUnifiedMovementComponent) override. The default implementations do
 * nothing, so a subclass only needs to override the inputs it supports.
 *
 * @ingroup gameplay
 */
UCLASS(Abstract)
class DT4MOB_API UBaseMovementComponent : public UPawnMovementComponent
{
    GENERATED_BODY()

public:
    /**
     * @brief Handles 2-axis movement input (e.g. WASD or gamepad left stick). Does nothing in the base class.
     * @param Input Normalised 2D input vector (X = forward, Y = right; negative values reverse).
     */
    virtual void HandleMoveInput(const FVector2D &Input) {}

    /**
     * @brief Handles 2-axis look input (e.g. mouse delta or right stick). Does nothing in the base class.
     * @param Input 2D input vector (X = yaw, Y = pitch).
     */
    virtual void HandleLookInput(const FVector2D &Input) {}

    /**
     * @brief Handles scalar zoom input (e.g. mouse wheel). Does nothing in the base class.
     * @param Value Positive zooms in, negative zooms out.
     */
    virtual void HandleZoomInput(float Value) {}
};
