// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DittoSecretsAsset.generated.h"

/**
 * @brief Holds Ditto connection credentials and endpoint config.
 *
 * Replaces Config/Secrets.ini. Instances of this asset must live outside of
 * source control (see .gitignore) and are only as protected as the project's
 * pak encryption settings (Project Settings > Packaging > Crypto) — cooking
 * alone does not encrypt the asset.
 */
UCLASS(BlueprintType)
class DT4MOB_API UDittoSecretsAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Ditto")
	FString Username;

	UPROPERTY(EditAnywhere, Category = "Ditto")
	FString Password;

	/** Ditto host, without scheme (e.g. "your-ditto-host.example.com"). */
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
