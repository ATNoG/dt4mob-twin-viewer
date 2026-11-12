// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/**
 * @file
 * @brief Declares UCredentialStoreService, which persists the login credentials in OS-protected storage.
 * @ingroup services
 */

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CredentialStoreService.generated.h"

/**
 * @brief GameInstance subsystem that persists the backend login (Host/Username/Password) in
 *        platform secure storage.
 *
 * Windows encrypts the data with DPAPI for the current Windows user and writes it to
 * `Saved/Credentials.dat`; macOS stores it as a Keychain generic password item. See
 * CredentialStoreService.cpp for the backends.
 *
 * OAuth access and refresh tokens are not stored here. UBackendConnectionService gets a new
 * token on every launch with a password grant using the stored Username/Password, so a saved
 * login stays usable until the user logs out instead of expiring with a refresh token.
 *
 * On platforms without a backend (including Linux), TryLoadCredentials() always returns false
 * and StoreCredentials() stores nothing, so the login screen is shown on every launch.
 *
 * @ingroup services
 */
UCLASS()
class DT4MOB_API UCredentialStoreService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @brief Loads and decrypts the stored credentials.
	 *
	 * The output parameters are only written on success.
	 *
	 * @param OutHost      Receives the stored backend host.
	 * @param OutUsername  Receives the stored username.
	 * @param OutPassword  Receives the stored password.
	 * @return             True if an entry existed, could be decrypted, and held exactly three
	 *                     fields. False otherwise, including on platforms without a backend.
	 */
	bool TryLoadCredentials(FString& OutHost, FString& OutUsername, FString& OutPassword) const;

	/**
	 * @brief Encrypts and stores the credentials, replacing any previous entry.
	 *
	 * Called by ULoginWidget after a successful login. Failures are only logged.
	 *
	 * @param Host      Backend host without scheme.
	 * @param Username  Login username.
	 * @param Password  Login password.
	 */
	void StoreCredentials(const FString& Host, const FString& Username, const FString& Password);

	/** @brief Deletes the stored entry, if any. Called on logout by AUnifiedController::Logout(). */
	void ClearCredentials();

	/**
	 * @brief Checks whether a stored entry exists, without decrypting it.
	 * @return True if the credential file (Windows) or Keychain item (macOS) exists; always
	 *         false on other platforms.
	 */
	bool HasStoredCredentials() const;

private:
	/**
	 * @brief Returns the path of the Windows credential file.
	 *
	 * Not called anywhere: the Windows backend builds the same path with its own file-local
	 * helper, and macOS has no file.
	 *
	 * @return FPaths::ProjectSavedDir() / "Credentials.dat".
	 */
	FString GetCredentialFilePath() const;
};
