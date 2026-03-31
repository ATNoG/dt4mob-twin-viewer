#pragma once

#include "CoreMinimal.h"
#include "CameraMode.generated.h"

/**
 * @brief Defines the available camera/control modes for the AUnifiedPawn.
 *
 * - RTS:     Top-down orthographic-style view with mouse-cursor panning, zooming,
 *            and click-to-select interaction. The mouse cursor is always visible.
 * - FreeFly: First-person free-fly navigation. Mouse look is active by default;
 *            the cursor can be unlocked via ToggleFreeFlyMouseUnlock() to allow
 *            cursor-based interaction while in this mode.
 */
UENUM(BlueprintType)
enum class ECameraMode : uint8
{
    RTS     UMETA(DisplayName = "RTS"),     ///< Real-time strategy top-down camera mode.
    FreeFly UMETA(DisplayName = "FreeFly")  ///< Free-flight first-person camera mode.
};
