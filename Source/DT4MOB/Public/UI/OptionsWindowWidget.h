#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "OptionsWindowWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UWidgetSwitcher;
class UOptionsAppearanceSectionWidget;
class UOptionsGeneralSectionWidget;

/**
 * @brief Full-screen modal settings window opened from the toolbar's Options button.
 *
 * Blocks interaction with the rest of the UI while open (a full-screen WidgetBorder catches
 * input, same role as LoginWidget's backdrop), with a left-side section sidebar — currently
 * "Appearance" and "General" (placeholder) — switching a UWidgetSwitcher, the same mechanism
 * EntityWindowWidget uses for its own (horizontal) tab bar. Logout/Exit live directly in the
 * sidebar, below the section list, rather than inside the General section.
 */
UCLASS()
class DT4MOB_API UOptionsWindowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    /** Shows the window and resets it to the Appearance section. */
    void Open();

    /** Hides the window. */
    void Close();

protected:
    /** Full-screen backdrop that blocks input to the rest of the UI. Must be named "WidgetBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* WidgetBorder;

    /** The centered modal panel itself. Must be named "WindowBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* WindowBorder;

    /** Must be named "CloseButton" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* CloseButton;

    /** Window title ("Options"). Must be named "TitleLabel" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* TitleLabel;

    // ── Sidebar ───────────────────────────────────────────────────────────────

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* AppearanceSectionButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* GeneralSectionButton;

    /** Background highlight for the active section button. Must be named "AppearanceSectionBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* AppearanceSectionBorder;

    /** Background highlight for the active section button. Must be named "GeneralSectionBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* GeneralSectionBorder;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* AppearanceSectionLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* GeneralSectionLabel;

    /** Separator line below the header. Must be named "Separator" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Separator;

    /** Vertical separator between the sidebar and the section content. Must be named "Separator2" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Separator2;

    // ── Sidebar bottom actions ───────────────────────────────────────────────────

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* LogoutButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* LogoutLabel;

    /** Background pill behind LogoutButton. Must be named "LogoutBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* LogoutBackground;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* ExitButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* ExitLabel;

    /** Background pill behind ExitButton. Must be named "ExitBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* ExitBackground;

    // ── Content ───────────────────────────────────────────────────────────────

    /** Section 0 = Appearance, Section 1 = General. Must be named "SectionSwitcher" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UWidgetSwitcher* SectionSwitcher;

    /** Must be the SectionSwitcher's index-0 child, named "AppearanceSection" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UOptionsAppearanceSectionWidget* AppearanceSection;

    /** Must be the SectionSwitcher's index-1 child, named "GeneralSection" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UOptionsGeneralSectionWidget* GeneralSection;

    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    int32 ActiveSectionIndex = 0;

    FLinearColor SectionActiveColor = FLinearColor::Transparent;
    FLinearColor SectionInactiveColor = FLinearColor::Transparent;

    FLinearColor ActionButtonNormalColor = FLinearColor::Transparent;
    FLinearColor ActionButtonHoverColor = FLinearColor::Transparent;
    FLinearColor ActionButtonPressedColor = FLinearColor::Transparent;

    void SwitchToSection(int32 Index);
    void RefreshSectionHighlight();
    void SetActionButtonColor(UBorder* Border, const FLinearColor& Color);

    UFUNCTION()
    void HandleAppearanceSectionClicked();

    UFUNCTION()
    void HandleGeneralSectionClicked();

    UFUNCTION()
    void HandleCloseClicked();

    UFUNCTION()
    void HandleLogoutClicked();

    UFUNCTION()
    void HandleExitClicked();

    UFUNCTION()
    void HandleLogoutHovered();

    UFUNCTION()
    void HandleLogoutUnhovered();

    UFUNCTION()
    void HandleLogoutPressed();

    UFUNCTION()
    void HandleLogoutReleased();

    UFUNCTION()
    void HandleExitHovered();

    UFUNCTION()
    void HandleExitUnhovered();

    UFUNCTION()
    void HandleExitPressed();

    UFUNCTION()
    void HandleExitReleased();
};
