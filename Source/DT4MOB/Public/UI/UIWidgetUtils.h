#pragma once

#include "CoreMinimal.h"

class UButton;

namespace UIWidgetUtils
{
    /** Strips the engine's default button brushes (the white outline) so only a backing Border shows through. */
    DT4MOB_API void StripDefaultButtonBrushes(UButton* Button);
}
