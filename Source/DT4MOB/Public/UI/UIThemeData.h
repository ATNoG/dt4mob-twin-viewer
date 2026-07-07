#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UIThemeData.generated.h"

/**
 * @brief A single UI theme preset — create one asset for Dark, one for Light.
 *
 * Assign both assets in Project Settings → Game → UI Theme Settings.
 * Switch the active theme at runtime via UUIThemeSubsystem::SetDarkMode().
 */
UCLASS(BlueprintType)
class DT4MOB_API UUIThemeData : public UDataAsset
{
    GENERATED_BODY()

public:
    // ── Backgrounds ──────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor BackgroundPrimary   = FLinearColor(0.055f, 0.055f, 0.055f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor BackgroundSecondary = FLinearColor(0.08f,  0.08f,  0.08f,  1.f);

    // ── Buttons ───────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    FLinearColor Hover               = FLinearColor(1.f, 1.f, 1.f, 0.06f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    FLinearColor Pressed             = FLinearColor(1.f, 1.f, 1.f, 0.12f);

    // ── Text ─────────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
    FLinearColor TextPrimary         = FLinearColor(0.85f, 0.85f, 0.85f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
    FLinearColor TextSecondary       = FLinearColor(0.45f, 0.45f, 0.45f, 1.f);

    // ── Accents ───────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accents")
    FLinearColor Accent              = FLinearColor(1.f, 0.4f, 0.13f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accents")
    FLinearColor Separator           = FLinearColor(1.f, 1.f, 1.f, 0.08f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accents")
    FLinearColor SelectionHighlight  = FLinearColor(1.f, 1.f, 1.f, 0.1f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accents")
    FLinearColor HighlightAccent     = FLinearColor(0.557f, 0.714f, 1.f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Accents")
    FLinearColor TabUnselected       = FLinearColor(0.45f, 0.45f, 0.45f, 1.f);
};
