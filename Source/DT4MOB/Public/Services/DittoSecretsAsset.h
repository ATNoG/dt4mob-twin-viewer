// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DittoSecretsAsset.generated.h"

/**
 * @brief Holds non-sensitive Ditto connection config (scheme, auth mode, WS start message).
 *
 * Contains no credentials: username/password are entered on the login screen and
 * persisted per-user by UCredentialStoreService (DPAPI / Keychain). Loaded once by
 * UBackendConnectionService::Initialize() from /Game/Data/DA_DittoSecrets.
 */
UCLASS(BlueprintType)
class DT4MOB_API UDittoSecretsAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Default Ditto host, without scheme — only used to prefill the login form. */
	UPROPERTY(EditAnywhere, Category = "Ditto")
	FString Host;

	UPROPERTY(EditAnywhere, Category = "Ditto")
	bool bUseHttps = true;

	/** When false, falls back to HTTP Basic auth (Base64 username:password). */
	UPROPERTY(EditAnywhere, Category = "Ditto")
	bool bUseOAuth = true;

	/** OAuth2 client_id used for the Keycloak password/refresh grant. */
	UPROPERTY(EditAnywhere, Category = "Ditto")
	FString OAuthClientId = TEXT("ditto");

	/** WebSocket START-SEND-EVENTS message sent on connect when no tile filter is active. */
	UPROPERTY(EditAnywhere, Category = "Ditto")
	FString WsStartMessage;
};
