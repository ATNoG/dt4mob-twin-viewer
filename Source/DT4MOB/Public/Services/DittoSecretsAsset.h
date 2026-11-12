// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/**
 * @file
 * @brief Declares UDittoSecretsAsset, the DataAsset with the non-sensitive backend connection settings.
 * @ingroup services
 */

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DittoSecretsAsset.generated.h"

/**
 * @brief DataAsset with the non-sensitive Ditto connection settings: default host, scheme,
 *        auth mode, OAuth client id and WebSocket start message.
 *
 * Holds no credentials. Username and password are typed on the login screen and persisted per
 * user by UCredentialStoreService (Windows DPAPI or macOS Keychain). UBackendConnectionService::Initialize()
 * loads it once from the fixed path `/Game/Data/DA_DittoSecrets`; the asset is git-ignored, so
 * each developer or deployment creates its own. Without it the service falls back to HTTPS +
 * OAuth with an empty client id and no start message.
 *
 * @ingroup services
 */
UCLASS(BlueprintType)
class DT4MOB_API UDittoSecretsAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** @brief Default backend host without scheme. Only used to prefill the host field of the login form. */
	UPROPERTY(EditAnywhere, Category = "Ditto")
	FString Host;

	/** @brief Use https:// for REST and wss:// for the WebSocket; false selects http:// and ws://. */
	UPROPERTY(EditAnywhere, Category = "Ditto")
	bool bUseHttps = true;

	/**
	 * @brief Authenticate with a Keycloak OAuth2 password grant. When false, every request uses
	 *        HTTP Basic auth (Base64 of username:password) instead.
	 */
	UPROPERTY(EditAnywhere, Category = "Ditto")
	bool bUseOAuth = true;

	/** @brief OAuth2 `client_id` sent with the Keycloak password and refresh grants. */
	UPROPERTY(EditAnywhere, Category = "Ditto")
	FString OAuthClientId = TEXT("ditto");

	/**
	 * @brief Command UWSService sends right after the socket connects when no event filter is
	 *        active (normally `START-SEND-EVENTS`). Also re-sent when the filter is cleared.
	 */
	UPROPERTY(EditAnywhere, Category = "Ditto")
	FString WsStartMessage;
};
