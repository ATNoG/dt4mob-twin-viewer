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
    FLinearColor BackgroundPrimary   = FLinearColor::FromSRGBColor(FColor(0x1D, 0x1D, 0x1D, 0xFF));

    /** Near-black backdrop, darker than BackgroundPrimary (e.g. full-screen login backdrop). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor BackgroundDeepest   = FLinearColor::FromSRGBColor(FColor(0x08, 0x08, 0x08, 0xFF));

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor BackgroundSecondary = FLinearColor(0.08f,  0.08f,  0.08f,  1.f);

    /** Toolbar bar, panel headers, and the active tab's background. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor PanelBackground     = FLinearColor::FromSRGBColor(FColor(0x23, 0x23, 0x23, 0xFF));

    /** Search box background (outline panel). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor SearchBackground    = FLinearColor::FromSRGBColor(FColor(0x16, 0x16, 0x16, 0xFF));

    /** Outline panel's scrollable list body background. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor ListBackground      = FLinearColor::FromSRGBColor(FColor(0x1D, 0x1D, 0x1D, 0xFF));

    /** Even-numbered row background (outline/assoc lists). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor RowBackgroundEven   = FLinearColor::FromSRGBColor(FColor(0x21, 0x21, 0x21, 0xFF));

    /** Inactive tab background (entity window tab bar). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor TabInactiveBackground = FLinearColor::FromSRGBColor(FColor(0x18, 0x18, 0x18, 0xFF));

    /** Border/outline stroke for window-style panels (e.g. entity window frame). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backgrounds")
    FLinearColor WindowOutline       = FLinearColor::FromSRGBColor(FColor(0x12, 0x12, 0x12, 0xFF));

    // ── Buttons ───────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    FLinearColor Hover               = FLinearColor(1.f, 1.f, 1.f, 0.06f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    FLinearColor Pressed             = FLinearColor(1.f, 1.f, 1.f, 0.12f);

    /** Idle button background — distinct from PanelBackground so buttons pop off the bar. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    FLinearColor ButtonIdle          = FLinearColor::FromSRGBColor(FColor(0x38, 0x38, 0x38, 0xFF));

    /** Row hover tint (outline list rows). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buttons")
    FLinearColor RowHover            = FLinearColor::FromSRGBColor(FColor(0x2E, 0x2E, 0x2E, 0xFF));

    // ── Text ─────────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
    FLinearColor TextPrimary         = FLinearColor::FromSRGBColor(FColor(0xDC, 0xDC, 0xDC, 0xFF));

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
    FLinearColor TextSecondary       = FLinearColor::FromSRGBColor(FColor(0x7A, 0x7A, 0x7A, 0xFF));

    /** "Active"-style status text (entity window). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
    FLinearColor StatusActive        = FLinearColor::FromSRGBColor(FColor(0x56, 0xCB, 0x78, 0xFF));

    /** Error/failure status text (e.g. login errors). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
    FLinearColor StatusError         = FLinearColor::FromSRGBColor(FColor(0xE0, 0x5C, 0x5C, 0xFF));

    /** "Open in Grafana" link text. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
    FLinearColor GrafanaLink         = FLinearColor::FromSRGBColor(FColor(0xF4, 0xA4, 0x60, 0xFF));

    /** Generic icon tint. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
    FLinearColor IconTint            = FLinearColor::FromSRGBColor(FColor(0xBF, 0xBF, 0xBF, 0xFF));

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
