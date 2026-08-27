/** @file LoginWidget.cpp
 *  @brief Implementation of ULoginWidget. All logic documentation is in the header.
 */
#include "UI/LoginWidget.h"
#include "Services/DittoService.h"
#include "Services/CredentialStoreService.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/GameInstance.h"
#include "Styling/SlateTypes.h"

namespace
{
	/** Strips a Button's own default brushes so only a Border placed behind it draws its chrome. */
	void StripButtonChrome(UButton* Button)
	{
		if (!Button) return;

		FSlateBrush Transparent;
		Transparent.DrawAs = ESlateBrushDrawType::NoDrawType;

		FButtonStyle Style = Button->GetStyle();
		Style.Normal = Transparent;
		Style.Hovered = Transparent;
		Style.Pressed = Transparent;
		Style.Disabled = Transparent;
		Button->SetStyle(Style);
	}
}

bool ULoginWidget::Initialize()
{
	if (!Super::Initialize())
		return false;

	if (UGameInstance* GI = GetGameInstance())
	{
		DittoService = GI->GetSubsystem<UDittoService>();
		CredentialStore = GI->GetSubsystem<UCredentialStoreService>();
	}

	if (LoginButton)
	{
		LoginButton->OnClicked.AddDynamic(this, &ULoginWidget::HandleLoginClicked);
		LoginButton->OnHovered.AddDynamic(this, &ULoginWidget::HandleLoginButtonHovered);
		LoginButton->OnUnhovered.AddDynamic(this, &ULoginWidget::HandleLoginButtonUnhovered);
		LoginButton->OnPressed.AddDynamic(this, &ULoginWidget::HandleLoginButtonPressed);
		LoginButton->OnReleased.AddDynamic(this, &ULoginWidget::HandleLoginButtonReleased);
		StripButtonChrome(LoginButton);
	}

	if (ExitButton)
	{
		ExitButton->OnClicked.AddDynamic(this, &ULoginWidget::HandleExitClicked);
		ExitButton->OnHovered.AddDynamic(this, &ULoginWidget::HandleExitButtonHovered);
		ExitButton->OnUnhovered.AddDynamic(this, &ULoginWidget::HandleExitButtonUnhovered);
		ExitButton->OnPressed.AddDynamic(this, &ULoginWidget::HandleExitButtonPressed);
		ExitButton->OnReleased.AddDynamic(this, &ULoginWidget::HandleExitButtonReleased);
		StripButtonChrome(ExitButton);
	}

	if (HostBox && DittoService && HostBox->GetText().IsEmpty())
		HostBox->SetText(FText::FromString(DittoService->GetDefaultHost()));

	SetError(FString());

	return true;
}

void ULoginWidget::HandleLoginClicked()
{
	if (!DittoService || !CredentialStore)
	{
		SetError(TEXT("Internal error: services unavailable."));
		return;
	}

	const FString Host = HostBox ? HostBox->GetText().ToString().TrimStartAndEnd() : FString();
	const FString Username = UsernameBox ? UsernameBox->GetText().ToString().TrimStartAndEnd() : FString();
	const FString Password = PasswordBox ? PasswordBox->GetText().ToString() : FString();

	if (Host.IsEmpty() || Username.IsEmpty() || Password.IsEmpty())
	{
		SetError(TEXT("Host, username and password are all required."));
		return;
	}

	SetError(FString());
	SetBusy(true);

	TWeakObjectPtr<ULoginWidget> WeakThis(this);
	DittoService->Login(Host, Username, Password,
		[WeakThis, Host, Username, Password](bool bSuccess)
		{
			ULoginWidget* Self = WeakThis.Get();
			if (!Self) return;

			Self->SetBusy(false);

			if (!bSuccess)
			{
				Self->SetError(TEXT("Login failed — check host, username and password."));
				return;
			}

			if (Self->CredentialStore)
				Self->CredentialStore->StoreCredentials(Host, Username, Password);

			Self->OnLoginSucceeded.Broadcast();
		});
}

void ULoginWidget::HandleExitClicked()
{
	UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);
}

void ULoginWidget::SetButtonBackgroundColor(UBorder* Background, const FLinearColor& Color)
{
	if (!Background) return;

	FSlateBrush Brush = Background->Background;
	Brush.TintColor = FSlateColor(Color);
	Brush.OutlineSettings.Color = FSlateColor(Color);
	Background->SetBrush(Brush);
}

void ULoginWidget::HandleLoginButtonHovered() { SetButtonBackgroundColor(LoginButtonBackground, ButtonHoverColor); }
void ULoginWidget::HandleLoginButtonUnhovered() { SetButtonBackgroundColor(LoginButtonBackground, ButtonNormalColor); }
void ULoginWidget::HandleLoginButtonPressed() { SetButtonBackgroundColor(LoginButtonBackground, ButtonPressedColor); }
void ULoginWidget::HandleLoginButtonReleased() { SetButtonBackgroundColor(LoginButtonBackground, ButtonHoverColor); }

void ULoginWidget::HandleExitButtonHovered() { SetButtonBackgroundColor(ExitButtonBackground, ButtonHoverColor); }
void ULoginWidget::HandleExitButtonUnhovered() { SetButtonBackgroundColor(ExitButtonBackground, ButtonNormalColor); }
void ULoginWidget::HandleExitButtonPressed() { SetButtonBackgroundColor(ExitButtonBackground, ButtonPressedColor); }
void ULoginWidget::HandleExitButtonReleased() { SetButtonBackgroundColor(ExitButtonBackground, ButtonHoverColor); }

void ULoginWidget::ApplyTheme_Implementation(UUIThemeData* Theme)
{
	if (!Theme) return;

	if (WidgetBorder)
		WidgetBorder->SetBrushColor(Theme->BackgroundDeepest);

	if (WindowBorder)
		WindowBorder->SetBrushColor(Theme->WindowOutline);

	if (BackgroundPattern)
		BackgroundPattern->SetColorAndOpacity(Theme->Separator);

	for (UBorder* FieldBorder : { HostBorder, UsernameBorder, PasswordBorder })
	{
		if (!FieldBorder) continue;

		FSlateBrush Brush = FieldBorder->Background;
		Brush.TintColor = FSlateColor(Theme->PanelBackground);
		Brush.OutlineSettings.Color = FSlateColor(Theme->WindowOutline);
		FieldBorder->SetBrush(Brush);
	}

	ButtonNormalColor = Theme->ButtonIdle;
	ButtonHoverColor = Theme->Hover;
	ButtonPressedColor = Theme->Pressed;

	SetButtonBackgroundColor(LoginButtonBackground, ButtonNormalColor);
	SetButtonBackgroundColor(ExitButtonBackground, ButtonNormalColor);

	if (ErrorText)
		ErrorText->SetColorAndOpacity(FSlateColor(Theme->StatusError));
}

void ULoginWidget::SetError(const FString& Message)
{
	if (!ErrorText)
		return;

	ErrorText->SetText(FText::FromString(Message));
	ErrorText->SetVisibility(Message.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

void ULoginWidget::SetBusy(bool bBusy)
{
	if (LoginButton)
		LoginButton->SetIsEnabled(!bBusy);
}
