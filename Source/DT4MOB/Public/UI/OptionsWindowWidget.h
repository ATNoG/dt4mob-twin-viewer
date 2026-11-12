/**
 * @file
 * @brief Declares UOptionsWindowWidget, the modal settings window opened from the toolbar.
 * @ingroup ui
 */
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
 * While open, a full-screen backdrop (WidgetBorder) catches input so the rest of the UI cannot
 * be used, like the backdrop of ULoginWidget. A sidebar on the left switches a UWidgetSwitcher
 * between the "Appearance" and "General" (placeholder) pages, the same mechanism
 * UEntityWindowWidget uses for its horizontal tab bar. Logout and Exit sit in the sidebar below
 * the section list rather than inside the General page.
 *
 * URootHUDWidget hosts the window and calls Open(); it stays in the widget tree and is only
 * collapsed when closed.
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UOptionsWindowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Binds the close, section, Logout and Exit buttons, strips the Logout and Exit
     *        buttons' default brushes and starts collapsed.
     * @return False if the base UUserWidget::Initialize() fails, true otherwise.
     */
    virtual bool Initialize() override;

    /**
     * @brief Selects switcher index 0, refreshes AppearanceSection's theme label and makes the window visible.
     * @see SectionSwitcher for how indices map to pages.
     */
    void Open();

    /** @brief Collapses the window. */
    void Close();

protected:
    /** @brief Full-screen backdrop that blocks input to the rest of the UI, tinted with the theme's BackgroundPrimary at 80% opacity. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* WidgetBorder;

    /** @brief Centered modal panel, tinted with the theme's WindowOutline. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* WindowBorder;

    /** @brief Closes the window. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* CloseButton;

    /** @brief Window title ("Options"). Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* TitleLabel;

    // --- Sidebar ---

    /** @brief Sidebar entry for the Appearance page. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* AppearanceSectionButton;

    /** @brief Sidebar entry for the General page. Required; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* GeneralSectionButton;

    /** @brief Background of the Appearance sidebar entry; SectionActiveColor while that page is selected. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* AppearanceSectionBorder;

    /** @brief Background of the General sidebar entry; SectionActiveColor while that page is selected. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* GeneralSectionBorder;

    /** @brief Caption of the Appearance sidebar entry. Optional. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* AppearanceSectionLabel;

    /** @brief Caption of the General sidebar entry. Optional. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* GeneralSectionLabel;

    /** @brief Separator line below the header. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Separator;

    /** @brief Vertical separator between the sidebar and the page content. Optional; bound by name from the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* Separator2;

    // --- Sidebar bottom actions ---

    /** @brief Logs out through AUnifiedController::Logout(), which clears the session and stored credentials and opens the login level. Optional. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* LogoutButton;

    /** @brief Caption of LogoutButton. Optional. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* LogoutLabel;

    /** @brief Border drawn behind LogoutButton; its color follows hover and press state. Optional. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* LogoutBackground;

    /** @brief Quits the application. Optional. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* ExitButton;

    /** @brief Caption of ExitButton. Optional. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* ExitLabel;

    /** @brief Border drawn behind ExitButton; its color follows hover and press state. Optional. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* ExitBackground;

    // --- Content ---

    /**
     * @brief Holds the section pages and shows one at a time. Required; bound by name from the widget Blueprint.
     *
     * @note The sidebar code treats index 1 as Appearance and index 0 as General (see
     *       HandleAppearanceSectionClicked() and RefreshSectionHighlight()), while Open() selects
     *       index 0. Which page Open() shows therefore depends on the child order in the widget
     *       Blueprint.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UWidgetSwitcher* SectionSwitcher;

    /** @brief Appearance page, a child of SectionSwitcher. Required; must be named "AppearanceSection" in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UOptionsAppearanceSectionWidget* AppearanceSection;

    /** @brief General page, a child of SectionSwitcher. Optional; must be named "GeneralSection" in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UOptionsGeneralSectionWidget* GeneralSection;

    /**
     * @brief Colors the backdrop, panel, labels, separators, sidebar entries and action buttons from Theme.
     * @param Theme  Active theme; nothing changes if it is null.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
    /** @brief Switcher index last passed to SwitchToSection(). */
    int32 ActiveSectionIndex = 0;

    /** @brief Sidebar entry background for the selected page (theme PanelBackground). */
    FLinearColor SectionActiveColor = FLinearColor::Transparent;
    /** @brief Sidebar entry background for the other pages (theme TabInactiveBackground). */
    FLinearColor SectionInactiveColor = FLinearColor::Transparent;

    /** @brief Logout and Exit background at rest (theme ButtonIdle). */
    FLinearColor ActionButtonNormalColor = FLinearColor::Transparent;
    /** @brief Logout and Exit background under the cursor (theme Hover). */
    FLinearColor ActionButtonHoverColor = FLinearColor::Transparent;
    /** @brief Logout and Exit background while pressed (theme Pressed). */
    FLinearColor ActionButtonPressedColor = FLinearColor::Transparent;

    /**
     * @brief Shows switcher page Index and updates the sidebar highlight.
     * @param Index  Child index in SectionSwitcher.
     */
    void SwitchToSection(int32 Index);
    /** @brief Highlights AppearanceSectionBorder when the active index is 1 and GeneralSectionBorder when it is 0. */
    void RefreshSectionHighlight();
    /**
     * @brief Sets the fill tint and the outline color of an action button background.
     * @param Border  Border to recolor; ignored if null.
     * @param Color   New fill and outline color.
     */
    void SetActionButtonColor(UBorder* Border, const FLinearColor& Color);

    /** @brief Switches to switcher index 1. */
    UFUNCTION()
    void HandleAppearanceSectionClicked();

    /** @brief Switches to switcher index 0. */
    UFUNCTION()
    void HandleGeneralSectionClicked();

    /** @brief Closes the window. */
    UFUNCTION()
    void HandleCloseClicked();

    /** @brief Calls AUnifiedController::Logout() on the owning player controller, if it is one. */
    UFUNCTION()
    void HandleLogoutClicked();

    /** @brief Quits the application with UKismetSystemLibrary::QuitGame(). */
    UFUNCTION()
    void HandleExitClicked();

    /** @brief Shows the hover color on LogoutBackground. */
    UFUNCTION()
    void HandleLogoutHovered();

    /** @brief Restores the normal color on LogoutBackground. */
    UFUNCTION()
    void HandleLogoutUnhovered();

    /** @brief Shows the pressed color on LogoutBackground. */
    UFUNCTION()
    void HandleLogoutPressed();

    /** @brief Puts LogoutBackground back to the hover color. */
    UFUNCTION()
    void HandleLogoutReleased();

    /** @brief Shows the hover color on ExitBackground. */
    UFUNCTION()
    void HandleExitHovered();

    /** @brief Restores the normal color on ExitBackground. */
    UFUNCTION()
    void HandleExitUnhovered();

    /** @brief Shows the pressed color on ExitBackground. */
    UFUNCTION()
    void HandleExitPressed();

    /** @brief Puts ExitBackground back to the hover color. */
    UFUNCTION()
    void HandleExitReleased();
};
