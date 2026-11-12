/**
 * @file
 * @brief Declares ECameraMode, the two camera/control modes of AUnifiedPawn.
 * @ingroup gameplay
 */

#pragma once

#include "CoreMinimal.h"
#include "CameraMode.generated.h"

/**
 * @brief Camera/control modes supported by AUnifiedPawn.
 *
 * In RTS mode the camera looks straight down from the end of a spring arm, zoom changes the
 * arm length, and the cursor stays visible for hover and selection. In FreeFly mode the arm
 * is collapsed and the mouse drives the view; the cursor is hidden until it is unlocked with
 * AUnifiedController::ToggleFreeFlyMouseUnlock(), which allows cursor interaction without
 * leaving FreeFly.
 *
 * @ingroup gameplay
 */
UENUM(BlueprintType)
enum class ECameraMode : uint8
{
    RTS     UMETA(DisplayName = "RTS"),     ///< Top-down camera on a variable-length spring arm, pans along the globe surface.
    FreeFly UMETA(DisplayName = "FreeFly")  ///< First-person flight with mouse look, kept upright relative to the globe.
};
