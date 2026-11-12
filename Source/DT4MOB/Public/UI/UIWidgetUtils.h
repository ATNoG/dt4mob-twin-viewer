/**
 * @file
 * @brief Small helpers shared by the UMG widgets.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"

class UButton;

/**
 * @brief Free helper functions for UMG widgets.
 * @ingroup ui
 */
namespace UIWidgetUtils
{
    /**
     * @brief Makes every state brush of a button invisible.
     *
     * Sets the Normal, Hovered, Pressed and Disabled brushes to draw nothing, which removes
     * the engine's default white outline. Widgets that draw their own themed background in a
     * Border behind the button call this when they bind the button's events, then tint that
     * Border on hover and press.
     *
     * @param Button  Button to restyle. Ignored if null.
     * @ingroup ui
     */
    DT4MOB_API void StripDefaultButtonBrushes(UButton* Button);
}
