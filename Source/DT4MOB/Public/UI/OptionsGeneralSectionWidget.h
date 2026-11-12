/**
 * @file
 * @brief Declares UOptionsGeneralSectionWidget, the placeholder "General" section of the Options window.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "OptionsGeneralSectionWidget.generated.h"

/**
 * @brief "General" section of the Options window.
 *
 * Has no logic yet. Logout and Exit moved to the Options window's sidebar (see
 * UOptionsWindowWidget); the class stays as the switcher page for future general settings.
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UOptionsGeneralSectionWidget : public UThemedWidget
{
    GENERATED_BODY()
};
