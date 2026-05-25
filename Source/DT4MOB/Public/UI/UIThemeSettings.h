#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UI/UIThemeData.h"
#include "UIThemeSettings.generated.h"

/**
 * @brief Project Settings → Game → UI Theme Settings.
 *
 * Assign your DA_ThemeDark and DA_ThemeLight assets here.
 * UUIThemeSubsystem reads these to know which assets to load.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "UI Theme Settings"))
class DT4MOB_API UUIThemeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UUIThemeSettings() { CategoryName = TEXT("Game"); }

    /** The theme used when dark mode is active. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Themes")
    TSoftObjectPtr<UUIThemeData> DarkTheme;

    /** The theme used when light mode is active. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Themes")
    TSoftObjectPtr<UUIThemeData> LightTheme;

    /** Theme applied on first launch before the user has made a choice. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Themes")
    bool bDefaultToDarkMode = true;
};
