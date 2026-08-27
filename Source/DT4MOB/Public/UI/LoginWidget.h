#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "LoginWidget.generated.h"

class UEditableTextBox;
class UButton;
class UTextBlock;
class UBorder;
class UImage;
class UDittoService;
class UCredentialStoreService;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoginSucceeded);

/**
 * @brief Login screen — collects Host/Username/Password, authenticates via UDittoService::Login,
 *        and on success persists the credentials via UCredentialStoreService so future launches
 *        can skip straight past this screen.
 *
 * AUnifiedController creates this on BeginPlay, before the main HUD, unless
 * UCredentialStoreService already has stored credentials (in which case it silently logs in and
 * this widget is never shown, see TrySilentLogin()).
 */
UCLASS()
class DT4MOB_API ULoginWidget : public UThemedWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	/** Broadcast once Login() succeeds and credentials have been persisted. */
	UPROPERTY(BlueprintAssignable)
	FOnLoginSucceeded OnLoginSucceeded;

protected:
	/** Full-screen backdrop behind the login box. Must be named "WidgetBorder" in the Blueprint layout. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* WidgetBorder;

	/** The centered login panel itself. Must be named "WindowBorder" in the Blueprint layout. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* WindowBorder;

	/** Tiled decorative pattern over the backdrop. Must be named "BackgroundPattern" in the Blueprint layout. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UImage* BackgroundPattern;

	/** Must be named "HostBox" in the Blueprint layout. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UEditableTextBox* HostBox;

	/** Background behind HostBox. Must be named "HostBorder" in the Blueprint layout. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* HostBorder;

	/** Must be named "UsernameBox" in the Blueprint layout. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UEditableTextBox* UsernameBox;

	/** Background behind UsernameBox. Must be named "UsernameBorder" in the Blueprint layout. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* UsernameBorder;

	/** Must be named "PasswordBox" in the Blueprint layout. Set "Is Password" in the Blueprint details panel. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UEditableTextBox* PasswordBox;

	/** Background behind PasswordBox. Must be named "PasswordBorder" in the Blueprint layout. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* PasswordBorder;

	/** Must be named "LoginButton" in the Blueprint layout. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UButton* LoginButton;

	/** Background behind LoginButton. Must be named "LoginButtonBackground" in the Blueprint layout. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* LoginButtonBackground;

	/** Must be named "ExitButton" in the Blueprint layout. Quits the application. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UButton* ExitButton;

	/** Background behind ExitButton. Must be named "ExitButtonBackground" in the Blueprint layout. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UBorder* ExitButtonBackground;

	/** Must be named "ErrorText" in the Blueprint layout. Hidden by default; shown on failure. */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock* ErrorText;

	virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

private:
	UPROPERTY()
	UDittoService* DittoService = nullptr;

	UPROPERTY()
	UCredentialStoreService* CredentialStore = nullptr;

	UFUNCTION()
	void HandleLoginClicked();

	UFUNCTION()
	void HandleExitClicked();

	FLinearColor ButtonNormalColor = FLinearColor::Transparent;
	FLinearColor ButtonHoverColor = FLinearColor::Transparent;
	FLinearColor ButtonPressedColor = FLinearColor::Transparent;

	static void SetButtonBackgroundColor(UBorder* Background, const FLinearColor& Color);

	UFUNCTION()
	void HandleLoginButtonHovered();
	UFUNCTION()
	void HandleLoginButtonUnhovered();
	UFUNCTION()
	void HandleLoginButtonPressed();
	UFUNCTION()
	void HandleLoginButtonReleased();

	UFUNCTION()
	void HandleExitButtonHovered();
	UFUNCTION()
	void HandleExitButtonUnhovered();
	UFUNCTION()
	void HandleExitButtonPressed();
	UFUNCTION()
	void HandleExitButtonReleased();

	void SetError(const FString& Message);
	void SetBusy(bool bBusy);
};
