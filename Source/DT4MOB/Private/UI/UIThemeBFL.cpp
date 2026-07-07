#include "UI/UIThemeBFL.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"

UUIThemeSubsystem* UUIThemeBFL::GetUIThemeSubsystem(const UObject* WorldContextObject)
{
    if (!WorldContextObject) return nullptr;

    UWorld* World = WorldContextObject->GetWorld();
    if (!World) return nullptr;

    UGameInstance* GI = World->GetGameInstance();
    if (!GI) return nullptr;

    // Use the first local player — fine for single-player / dedicated-client builds.
    ULocalPlayer* LP = GI->GetLocalPlayerByIndex(0);
    if (!LP) return nullptr;

    return LP->GetSubsystem<UUIThemeSubsystem>();
}

UUIThemeData* UUIThemeBFL::GetActiveTheme(const UObject* WorldContextObject)
{
    UUIThemeSubsystem* Sub = GetUIThemeSubsystem(WorldContextObject);
    return Sub ? Sub->GetTheme() : nullptr;
}
