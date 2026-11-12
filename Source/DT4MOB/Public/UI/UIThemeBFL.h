/**
 * @file
 * @brief Blueprint function library that gives widgets quick access to the UI theme.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/UIThemeSubsystem.h"
#include "UI/UIThemeData.h"
#include "UIThemeBFL.generated.h"

/**
 * @brief Static helpers that reach UUIThemeSubsystem from any Blueprint or C++ widget.
 *
 * The subsystem lives on the local player, so getting to it from a widget normally takes
 * several nodes. UThemedWidget uses GetUIThemeSubsystem() to bind to theme changes, and
 * Blueprints can call GetActiveTheme() to read the current colours directly.
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UUIThemeBFL : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * @brief Returns the theme subsystem of the first local player.
     *
     * @param WorldContextObject  Any object with a valid world (usually the calling widget).
     * @return The subsystem, or nullptr if the object, its world, the game instance or local
     *         player 0 is missing.
     */
    UFUNCTION(BlueprintPure, Category = "UI|Theme", meta = (WorldContext = "WorldContextObject"))
    static UUIThemeSubsystem* GetUIThemeSubsystem(const UObject* WorldContextObject);

    /**
     * @brief Returns the theme data asset that is active right now.
     *
     * @param WorldContextObject  Any object with a valid world (usually the calling widget).
     * @return The active theme, or nullptr if the subsystem is not available or no asset is
     *         assigned for the current mode in UUIThemeSettings.
     */
    UFUNCTION(BlueprintPure, Category = "UI|Theme", meta = (WorldContext = "WorldContextObject"))
    static UUIThemeData* GetActiveTheme(const UObject* WorldContextObject);
};
