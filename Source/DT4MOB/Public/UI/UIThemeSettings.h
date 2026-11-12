/**
 * @file
 * @brief Project settings page that selects the dark and light theme assets.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "UI/UIThemeData.h"
#include "UIThemeSettings.generated.h"

/**
 * @brief Project-wide theme configuration, shown under Project Settings -> Game -> UI Theme
 *        Settings.
 *
 * Values are saved to the [/Script/DT4MOB.UIThemeSettings] section of
 * Config/DefaultGame.ini, which currently points at DA_ThemeDark and DA_ThemeLight in
 * /Game/Blueprints/UI. UUIThemeSubsystem reads the class default object to decide which
 * asset to load.
 *
 * @ingroup ui
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "UI Theme Settings"))
class DT4MOB_API UUIThemeSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** @brief Lists this page under the "Game" category in Project Settings. */
    UUIThemeSettings() { CategoryName = TEXT("Game"); }

    /** @brief Theme asset loaded while dark mode is active. Edited in Project Settings. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Themes")
    TSoftObjectPtr<UUIThemeData> DarkTheme;

    /** @brief Theme asset loaded while light mode is active. Edited in Project Settings. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Themes")
    TSoftObjectPtr<UUIThemeData> LightTheme;

    /**
     * @brief Mode used when the user has no saved preference yet (first launch). Edited in
     *        Project Settings.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Themes")
    bool bDefaultToDarkMode = true;
};
