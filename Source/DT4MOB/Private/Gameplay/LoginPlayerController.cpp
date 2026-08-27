/** @file LoginPlayerController.cpp
 *  @brief Implementation of ALoginPlayerController. All logic documentation is in the header.
 */
#include "Gameplay/LoginPlayerController.h"
#include "UI/LoginWidget.h"
#include "Services/DittoService.h"
#include "Services/CredentialStoreService.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"

void ALoginPlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = true;

	TrySilentLoginOrShowLoginScreen();
}

void ALoginPlayerController::TrySilentLoginOrShowLoginScreen()
{
	UGameInstance* GI = GetGameInstance();
	UDittoService* DittoSvc = GI ? GI->GetSubsystem<UDittoService>() : nullptr;
	UCredentialStoreService* CredStore = GI ? GI->GetSubsystem<UCredentialStoreService>() : nullptr;

	if (!DittoSvc || !CredStore)
	{
		UE_LOG(LogTemp, Error, TEXT("ALoginPlayerController: DittoService/CredentialStoreService unavailable, cannot log in"));
		ShowLoginWidget();
		return;
	}

	FString Host, Username, Password;
	if (!CredStore->TryLoadCredentials(Host, Username, Password))
	{
		ShowLoginWidget();
		return;
	}

	TWeakObjectPtr<ALoginPlayerController> WeakThis(this);
	DittoSvc->Login(Host, Username, Password,
		[WeakThis](bool bSuccess)
		{
			ALoginPlayerController* Self = WeakThis.Get();
			if (!Self) return;

			if (bSuccess)
				Self->GoToMainScene();
			else
				Self->ShowLoginWidget();
		});
}

void ALoginPlayerController::ShowLoginWidget()
{
	if (!LoginWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("ALoginPlayerController: LoginWidgetClass not set, cannot show login screen"));
		return;
	}

	if (ActiveLoginWidget)
		return; // Already showing (e.g. a stray second BeginPlay/auth callback).

	ActiveLoginWidget = CreateWidget<ULoginWidget>(this, LoginWidgetClass);
	if (!ActiveLoginWidget)
		return;

	ActiveLoginWidget->OnLoginSucceeded.AddDynamic(this, &ALoginPlayerController::HandleLoginSucceeded);
	ActiveLoginWidget->AddToViewport();
}

void ALoginPlayerController::HandleLoginSucceeded()
{
	GoToMainScene();
}

void ALoginPlayerController::GoToMainScene()
{
	UGameplayStatics::OpenLevel(this, MainSceneLevelName);
}
