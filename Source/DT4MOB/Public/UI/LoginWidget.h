/**
 * @file
 * @brief Declares ULoginWidget, the login screen shown when no silent login is possible.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "LoginWidget.generated.h"

class UEditableTextBox;
class UButton;
class UTextBlock;
class UBorder;
class UImage;
class UBackendConnectionService;
class UCredentialStoreService;

/**
 * @brief Fires after a manual login succeeds and the credentials have been passed to UCredentialStoreService.
 * @ingroup ui
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoginSucceeded);

/**
 * @brief Login screen: the user enters host, username and password, and the widget authenticates
 *        them with UBackendConnectionService::Login().
 *
 * ALoginPlayerController creates this widget only when a silent login is not possible, that is
 * when UCredentialStoreService has no stored credentials or logging in with them fails (see
 * ALoginPlayerController::TrySilentLoginOrShowLoginScreen()). Login() authenticates against
 * Keycloak with an OAuth2 password grant, or uses HTTP Basic auth when the backend is configured
 * without OAuth.
 *
 * On success the widget saves host, username and password with
 * UCredentialStoreService::StoreCredentials(), which encrypts them for the current OS user (DPAPI
 * on Windows, Keychain on macOS), so the next launch can log in without showing this screen. It
 * then broadcasts OnLoginSucceeded. On failure it shows a message in ErrorText and enables the
 * login button again.
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API ULoginWidget : public UThemedWidget
{
	GENERATED_BODY()

public:
	/**
	 * @brief Caches the backend and credential services, binds the button events, strips the
	 *        buttons' default brushes and prefills an empty HostBox with
	 *        UBackendConnectionService::GetDefaultHost().
	 * @return False if the base UUserWidget::Initialize() fails, true otherwise.
	 */
	virtual bool Initialize() override;

	/** @brief Broadcast after Login() reports success and the credentials have been stored. ALoginPlayerController binds it to open the main level. */
	UPROPERTY(BlueprintAssignable)
	FOnLoginSucceeded OnLoginSucceeded;

protected:
	/** @brief Full-screen backdrop behind the login panel, tinted with the theme's BackgroundDeepest. Optional; bound by name from the widget Blueprint. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* WidgetBorder;

	/** @brief Centered login panel, tinted with the theme's WindowOutline. Optional; bound by name from the widget Blueprint. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* WindowBorder;

	/** @brief Decorative pattern drawn over the backdrop, tinted with the theme's Separator color. Optional; bound by name from the widget Blueprint. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* BackgroundPattern;

	/** @brief Text box for the Ditto host, without scheme. Prefilled from the DA_DittoSecrets default host. Required; bound by name from the widget Blueprint. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UEditableTextBox* HostBox;

	/** @brief Field background behind HostBox; fill and outline follow the theme. Optional; bound by name from the widget Blueprint. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* HostBorder;

	/** @brief Text box for the username. Required; bound by name from the widget Blueprint. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UEditableTextBox* UsernameBox;

	/** @brief Field background behind UsernameBox. Optional; bound by name from the widget Blueprint. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* UsernameBorder;

	/** @brief Text box for the password. Required; bound by name from the widget Blueprint, where "Is Password" must be enabled so the text is masked. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UEditableTextBox* PasswordBox;

	/** @brief Field background behind PasswordBox. Optional; bound by name from the widget Blueprint. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* PasswordBorder;

	/** @brief Submits the form. Disabled while a login request is pending. Required; bound by name from the widget Blueprint. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* LoginButton;

	/** @brief Border drawn behind LoginButton. Its color follows hover and press state, since the button's own brushes are stripped. Optional. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* LoginButtonBackground;

	/** @brief Quits the application. Optional; bound by name from the widget Blueprint. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* ExitButton;

	/** @brief Border drawn behind ExitButton, colored like LoginButtonBackground. Optional. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* ExitButtonBackground;

	/** @brief Validation or login error message, in the theme's StatusError color. Collapsed while there is no message. Optional. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* ErrorText;

	/**
	 * @brief Colors the backdrop, panel, pattern, field borders, button backgrounds and error text from Theme.
	 * @param Theme  Active theme; nothing changes if it is null.
	 */
	virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
	/** @brief Service that performs the login request. Null if no GameInstance was available in Initialize(). */
	UPROPERTY()
	UBackendConnectionService* BackendService = nullptr;

	/** @brief Service that saves the credentials after a successful login. Null if no GameInstance was available in Initialize(). */
	UPROPERTY()
	UCredentialStoreService* CredentialStore = nullptr;

	/**
	 * @brief Validates the form and starts UBackendConnectionService::Login().
	 *
	 * Host and username are trimmed; the password is sent as typed. If a service is missing or a
	 * field is empty, it shows an error and sends nothing. While the request runs the login
	 * button is disabled. The completion callback holds only a weak pointer to this widget, so
	 * it does nothing if the widget is gone by then.
	 */
	UFUNCTION()
	void HandleLoginClicked();

	/** @brief Quits the application with UKismetSystemLibrary::QuitGame(). */
	UFUNCTION()
	void HandleExitClicked();

	/** @brief Button background color at rest (theme ButtonIdle). */
	FLinearColor ButtonNormalColor = FLinearColor::Transparent;
	/** @brief Button background color under the cursor (theme Hover). */
	FLinearColor ButtonHoverColor = FLinearColor::Transparent;
	/** @brief Button background color while pressed (theme Pressed). */
	FLinearColor ButtonPressedColor = FLinearColor::Transparent;

	/**
	 * @brief Sets the fill tint and the outline color of a button background border.
	 * @param Background  Border to recolor; ignored if null.
	 * @param Color       New fill and outline color.
	 */
	static void SetButtonBackgroundColor(UBorder* Background, const FLinearColor& Color);

	/** @brief Shows the hover color on LoginButtonBackground. */
	UFUNCTION()
	void HandleLoginButtonHovered();
	/** @brief Restores the normal color on LoginButtonBackground. */
	UFUNCTION()
	void HandleLoginButtonUnhovered();
	/** @brief Shows the pressed color on LoginButtonBackground. */
	UFUNCTION()
	void HandleLoginButtonPressed();
	/** @brief Puts LoginButtonBackground back to the hover color. */
	UFUNCTION()
	void HandleLoginButtonReleased();

	/** @brief Shows the hover color on ExitButtonBackground. */
	UFUNCTION()
	void HandleExitButtonHovered();
	/** @brief Restores the normal color on ExitButtonBackground. */
	UFUNCTION()
	void HandleExitButtonUnhovered();
	/** @brief Shows the pressed color on ExitButtonBackground. */
	UFUNCTION()
	void HandleExitButtonPressed();
	/** @brief Puts ExitButtonBackground back to the hover color. */
	UFUNCTION()
	void HandleExitButtonReleased();

	/**
	 * @brief Shows Message in ErrorText, or collapses ErrorText when Message is empty.
	 * @param Message  Text to show; an empty string hides the error.
	 */
	void SetError(const FString& Message);

	/**
	 * @brief Disables LoginButton while a login request is pending.
	 * @param bBusy  True while waiting for Login() to complete.
	 */
	void SetBusy(bool bBusy);
};
