#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/UIThemeSubsystem.h"
#include "UI/UIThemeData.h"
#include "UIThemeBFL.generated.h"

/**
 * Static helpers so any Blueprint can reach UIThemeSubsystem without a complex node chain.
 * Call GetActiveTheme() in Event Construct to get the current colours.
 */
UCLASS()
class DT4MOB_API UUIThemeBFL : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Returns the UIThemeSubsystem for the local player, or null if not available. */
    UFUNCTION(BlueprintPure, Category = "UI|Theme", meta = (WorldContext = "WorldContextObject"))
    static UUIThemeSubsystem* GetUIThemeSubsystem(const UObject* WorldContextObject);

    /** Shortcut: returns the active theme data asset directly. Null if subsystem not ready. */
    UFUNCTION(BlueprintPure, Category = "UI|Theme", meta = (WorldContext = "WorldContextObject"))
    static UUIThemeData* GetActiveTheme(const UObject* WorldContextObject);
};
