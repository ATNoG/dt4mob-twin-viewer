#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LoginPlayerController.generated.h"

class ULoginWidget;

/**
 * @brief PlayerController for the dedicated login level.
 *
 * On BeginPlay, tries a silent login using credentials from UCredentialStoreService. On success
 * (or once the user manually completes ULoginWidget), opens the main scene level. Fully
 * decoupled from AUnifiedController — no pawn, no Cesium/camera setup — so none of the main
 * scene's cost is paid until the user is actually authenticated.
 */
UCLASS()
class DT4MOB_API ALoginPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** @brief Blueprint-assignable class for the login widget. */
	UPROPERTY(EditAnywhere, Category = "Login")
	TSubclassOf<ULoginWidget> LoginWidgetClass;

	/** @brief Name of the main scene level to load once authenticated. */
	UPROPERTY(EditAnywhere, Category = "Login")
	FName MainSceneLevelName = TEXT("Scene");

protected:
	virtual void BeginPlay() override;

private:
	/** @brief Tries stored credentials first; falls back to ShowLoginWidget() if none exist or the attempt fails. */
	void TrySilentLoginOrShowLoginScreen();

	/** @brief Creates and displays the login widget, wiring its success delegate to GoToMainScene(). */
	void ShowLoginWidget();

	/** @brief Callback for ULoginWidget::OnLoginSucceeded; dynamic delegates need a UFUNCTION. */
	UFUNCTION()
	void HandleLoginSucceeded();

	/** @brief Opens MainSceneLevelName now that DittoService is authenticated. */
	void GoToMainScene();

	UPROPERTY()
	ULoginWidget* ActiveLoginWidget = nullptr;
};
