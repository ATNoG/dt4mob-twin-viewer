/**
 * @file
 * @brief Declares ALoginPlayerController, which runs the login flow before the main scene loads.
 * @ingroup gameplay
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LoginPlayerController.generated.h"

class ULoginWidget;

/**
 * @brief Player controller for the dedicated login level.
 *
 * On BeginPlay it tries a silent login with credentials stored by UCredentialStoreService.
 * When that succeeds, or once the user completes ULoginWidget, it opens MainSceneLevelName.
 * It shares nothing with AUnifiedController (no pawn, no Cesium or camera setup), so none of
 * the main scene's cost is paid until the user is authenticated.
 *
 * @ingroup gameplay
 */
UCLASS()
class DT4MOB_API ALoginPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** @brief Login widget class to create when a silent login is not possible. Set in the controller Blueprint or defaults. */
	UPROPERTY(EditAnywhere, Category = "Login")
	TSubclassOf<ULoginWidget> LoginWidgetClass;

	/** @brief Level opened with UGameplayStatics::OpenLevel() once the user is authenticated. */
	UPROPERTY(EditAnywhere, Category = "Login")
	FName MainSceneLevelName = TEXT("/Game/Scenes/WorldMap/Scene");

protected:
	/** @brief Switches to UI-only input with a visible cursor, then starts TrySilentLoginOrShowLoginScreen(). */
	virtual void BeginPlay() override;

private:
	/**
	 * @brief Logs in with stored credentials if there are any, otherwise shows the login widget.
	 *
	 * Falls back to ShowLoginWidget() when the backend or credential subsystem is missing, when no
	 * credentials are stored, or when UBackendConnectionService::Login() reports failure. On
	 * success it calls GoToMainScene(). The login callback holds a weak pointer, so it does nothing
	 * if the controller has been destroyed in the meantime.
	 */
	void TrySilentLoginOrShowLoginScreen();

	/**
	 * @brief Creates the login widget, binds its OnLoginSucceeded delegate and adds it to the viewport.
	 *
	 * Does nothing if a widget is already showing. Logs an error and returns if LoginWidgetClass
	 * is not set, which leaves the user on an empty screen.
	 */
	void ShowLoginWidget();

	/** @brief Bound to ULoginWidget::OnLoginSucceeded (dynamic delegates need a UFUNCTION); calls GoToMainScene(). */
	UFUNCTION()
	void HandleLoginSucceeded();

	/** @brief Opens MainSceneLevelName. Called once UBackendConnectionService is authenticated. */
	void GoToMainScene();

	/** @brief Login widget currently on screen, or nullptr if none has been created. Guards against creating a second one. */
	UPROPERTY()
	ULoginWidget* ActiveLoginWidget = nullptr;
};
