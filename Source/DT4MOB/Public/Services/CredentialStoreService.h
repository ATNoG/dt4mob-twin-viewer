// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CredentialStoreService.generated.h"

/**
 * @brief GameInstance subsystem that persists Ditto login credentials (Host/Username/Password)
 *        via a platform-specific secure-storage backend (see CredentialStoreService.cpp):
 *        Windows DPAPI or macOS Keychain Services.
 *
 * OAuth access/refresh tokens are deliberately NOT persisted here — they are short-lived and
 * DittoService re-derives them each launch via a fresh password grant using the stored
 * Username/Password. This is what lets a login stay valid indefinitely until the user logs out,
 * rather than expiring whenever a persisted refresh token eventually goes stale.
 *
 * On any platform without an implemented backend, TryLoadCredentials() always returns false and
 * StoreCredentials() is a no-op, which simply means the login screen is shown on every launch.
 */
UCLASS()
class DT4MOB_API UCredentialStoreService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @brief Attempts to load and decrypt previously stored credentials.
	 * @return True if a credential file existed and was successfully decrypted.
	 */
	bool TryLoadCredentials(FString& OutHost, FString& OutUsername, FString& OutPassword) const;

	/** @brief Encrypts and writes Host/Username/Password to disk, overwriting any previous entry. */
	void StoreCredentials(const FString& Host, const FString& Username, const FString& Password);

	/** @brief Deletes the stored credential file, if any. Called on Logout. */
	void ClearCredentials();

	/** @brief True if a credential file is present on disk (does not attempt to decrypt it). */
	bool HasStoredCredentials() const;

private:
	FString GetCredentialFilePath() const;
};
