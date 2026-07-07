#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/UIThemeData.h"
#include "UIThemeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnThemeChanged, UUIThemeData*, NewTheme);

/**
 * @brief Manages the active UI theme and persists the user's preference.
 *
 * Widgets call GetTheme() to read colours.
 * Call SetDarkMode() at runtime to switch — OnThemeChanged fires so all
 * open widgets can refresh themselves.
 *
 * The dark/light preference is saved to GameUserSettings and restored on
 * next launch automatically.
 */
UCLASS()
class DT4MOB_API UUIThemeSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Fires whenever the active theme changes. Widgets bind here to refresh colours. */
    UPROPERTY(BlueprintAssignable)
    FOnThemeChanged OnThemeChanged;

    /** Returns the currently active theme data asset. Never null after Initialize(). */
    UFUNCTION(BlueprintPure, Category = "UI|Theme")
    UUIThemeData* GetTheme() const { return ActiveTheme; }

    /** Returns true if dark mode is currently active. */
    UFUNCTION(BlueprintPure, Category = "UI|Theme")
    bool IsDarkMode() const { return bDarkMode; }

    /**
     * @brief Switches between dark and light mode, saves the preference, and
     *        broadcasts OnThemeChanged so all widgets can update.
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Theme")
    void SetDarkMode(bool bInDarkMode);

    /** Convenience toggle. */
    UFUNCTION(BlueprintCallable, Category = "UI|Theme")
    void ToggleTheme() { SetDarkMode(!bDarkMode); }

private:
    UPROPERTY()
    UUIThemeData* ActiveTheme = nullptr;

    bool bDarkMode = true;

    void ApplyTheme();
    void SavePreference() const;
    bool LoadPreference() const;
};
