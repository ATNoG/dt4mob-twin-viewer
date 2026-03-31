#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PawnMovementComponent.h"
#include "BaseMovementComponent.generated.h"

/**
 * @brief Abstract base class for pawn movement components used by AUnifiedPawn.
 *
 * Declares the input-handling interface that concrete movement implementations
 * (e.g. UUnifiedMovementComponent) must override. Default implementations are
 * empty no-ops so that partial overrides remain valid.
 */
UCLASS(Abstract)
class DT4MOB_API UBaseMovementComponent : public UPawnMovementComponent
{
    GENERATED_BODY()

public:
    /**
     * @brief Handles 2-axis movement input (e.g. WASD or gamepad left stick).
     * @param Input Normalised 2D input vector (X = forward/back, Y = strafe left/right).
     */
    virtual void HandleMoveInput(const FVector2D &Input) {}

    /**
     * @brief Handles 2-axis look/camera rotation input (e.g. mouse delta or right stick).
     * @param Input 2D input vector (X = yaw, Y = pitch).
     */
    virtual void HandleLookInput(const FVector2D &Input) {}

    /**
     * @brief Handles scalar zoom input (e.g. mouse wheel).
     * @param Value Positive = zoom in, negative = zoom out.
     */
    virtual void HandleZoomInput(float Value) {}
};
