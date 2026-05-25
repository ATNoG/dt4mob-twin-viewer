#include "UI/UIThemeSubsystem.h"
#include "UI/UIThemeSettings.h"
#include "GameFramework/GameUserSettings.h"

static const FString ThemePrefSection = TEXT("UITheme");
static const FString ThemePrefKey     = TEXT("bDarkMode");

void UUIThemeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    bDarkMode = LoadPreference();
    ApplyTheme();
}

void UUIThemeSubsystem::SetDarkMode(bool bInDarkMode)
{
    if (bDarkMode == bInDarkMode) return;

    bDarkMode = bInDarkMode;
    ApplyTheme();
    SavePreference();
    OnThemeChanged.Broadcast(ActiveTheme);
}

void UUIThemeSubsystem::ApplyTheme()
{
    const UUIThemeSettings* Settings = GetDefault<UUIThemeSettings>();
    if (!Settings) return;

    const TSoftObjectPtr<UUIThemeData>& Ref = bDarkMode ? Settings->DarkTheme : Settings->LightTheme;
    ActiveTheme = Ref.LoadSynchronous();

    if (!ActiveTheme)
    {
        UE_LOG(LogTemp, Warning, TEXT("UIThemeSubsystem: %s theme asset not set in Project Settings → Game → UI Theme Settings"),
               bDarkMode ? TEXT("Dark") : TEXT("Light"));
    }
}

void UUIThemeSubsystem::SavePreference() const
{
    GConfig->SetBool(*ThemePrefSection, *ThemePrefKey, bDarkMode, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni);
}

bool UUIThemeSubsystem::LoadPreference() const
{
    const UUIThemeSettings* Settings = GetDefault<UUIThemeSettings>();
    const bool bDefault = Settings ? Settings->bDefaultToDarkMode : true;

    bool bSaved = bDefault;
    GConfig->GetBool(*ThemePrefSection, *ThemePrefKey, bSaved, GGameUserSettingsIni);
    return bSaved;
}
