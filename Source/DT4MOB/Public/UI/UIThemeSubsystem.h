/**
 * @file
 * @brief Local player subsystem that owns the active UI theme and the dark/light preference.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/UIThemeData.h"
#include "UIThemeSubsystem.generated.h"

/**
 * @brief Fires when UUIThemeSubsystem::SetDarkMode() switches to the other theme.
 * @param NewTheme  The theme asset that is now active. Null if no asset is assigned for the
 *                  new mode in UUIThemeSettings.
 * @ingroup ui
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThemeChanged, UUIThemeData*, NewTheme);

/**
 * @brief Holds the active UI theme asset and remembers whether the user chose dark or light
 *        mode.
 *
 * Widgets read colours with GetTheme() (or UUIThemeBFL::GetActiveTheme()). UThemedWidget
 * binds to OnThemeChanged on construct, so every open themed widget repaints when
 * SetDarkMode() switches modes.
 *
 * The preference is stored as bDarkMode in the [UITheme] section of GameUserSettings.ini
 * and restored when the local player is created. Without a saved value the subsystem falls
 * back to UUIThemeSettings::bDefaultToDarkMode.
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UUIThemeSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    /**
     * @brief Restores the saved dark/light preference and loads the matching theme asset.
     * @param Collection  Subsystem collection that is initializing this subsystem.
     */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** @brief Broadcast after the active theme changes. Widgets bind here to reapply colours. */
    UPROPERTY(BlueprintAssignable)
    FOnThemeChanged OnThemeChanged;

    /**
     * @brief Returns the active theme data asset.
     * @return The active theme, or nullptr if the asset for the current mode is not assigned
     *         in UUIThemeSettings (a warning is logged in that case).
     */
    UFUNCTION(BlueprintPure, Category = "UI|Theme")
    UUIThemeData* GetTheme() const { return ActiveTheme; }

    /**
     * @brief Reports which mode is selected.
     * @return True for dark mode, false for light mode.
     */
    UFUNCTION(BlueprintPure, Category = "UI|Theme")
    bool IsDarkMode() const { return bDarkMode; }

    /**
     * @brief Switches between dark and light mode.
     *
     * Loads the matching theme asset, writes the preference to GameUserSettings.ini and
     * broadcasts OnThemeChanged. Does nothing if the requested mode is already active.
     *
     * @param bInDarkMode  True for dark mode, false for light mode.
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Theme")
    void SetDarkMode(bool bInDarkMode);

    /** @brief Switches to the other mode through SetDarkMode(). */
    UFUNCTION(BlueprintCallable, Category = "UI|Theme")
    void ToggleTheme() { SetDarkMode(!bDarkMode); }

private:
    /** @brief Theme asset for the current mode, loaded by ApplyTheme(). Null if not assigned. */
    UPROPERTY()
    UUIThemeData* ActiveTheme = nullptr;

    /** @brief Current mode: true for dark, false for light. */
    bool bDarkMode = true;

    /**
     * @brief Synchronously loads the theme asset for bDarkMode from UUIThemeSettings into
     *        ActiveTheme. Logs a warning if that asset is not assigned.
     */
    void ApplyTheme();

    /** @brief Writes bDarkMode to GameUserSettings.ini and flushes the file. */
    void SavePreference() const;

    /**
     * @brief Reads the saved mode from GameUserSettings.ini.
     * @return The saved value, or UUIThemeSettings::bDefaultToDarkMode if nothing is saved.
     */
    bool LoadPreference() const;
};
