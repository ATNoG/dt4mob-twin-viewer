// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Implementation of UCredentialStoreService and its per-platform secret storage backends.
 *
 * Each platform implements the same four file-local functions (PlatformStoreSecret,
 * PlatformLoadSecret, PlatformDeleteSecret, PlatformHasSecret), selected at compile time:
 *
 *  - Windows: DPAPI (CryptProtectData/CryptUnprotectData), tied to the logged-in Windows user.
 *    The encrypted blob is written to `Saved/Credentials.dat`.
 *  - macOS: Keychain Services (SecItemAdd/SecItemCopyMatching/SecItemDelete). There is no
 *    file; the keychain is the storage and encrypts at rest itself.
 *  - Any other platform (including Linux): stubs that store nothing and never find an entry,
 *    so the login screen is shown on every launch.
 *
 * The stored plaintext is "Host\nUsername\nPassword" encoded as UTF-8.
 *
 * @ingroup services
 */
#include "Services/CredentialStoreService.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <wincrypt.h>
#include "Windows/HideWindowsPlatformTypes.h"
#elif PLATFORM_MAC
#include <Security/Security.h>
#endif

namespace
{
	/**
	 * @brief Separator between Host, Username and Password in the stored plaintext.
	 *
	 * None of the three values is expected to contain a newline. If one does, loading fails
	 * because the data no longer splits into exactly three fields.
	 */
	const TCHAR* FieldDelimiter = TEXT("\n");

#if PLATFORM_WINDOWS

	/**
	 * @brief Encrypts a byte buffer with DPAPI for the current Windows user.
	 *
	 * Uses CryptProtectData with no extra entropy and CRYPTPROTECT_UI_FORBIDDEN, so only the
	 * same user on the same machine can decrypt the result.
	 *
	 * @param PlainBytes         Data to encrypt.
	 * @param OutEncryptedBytes  Receives the encrypted blob on success.
	 * @return                   True on success; false (with an error logged) if DPAPI fails.
	 */
	bool DpapiEncrypt(const TArray<uint8>& PlainBytes, TArray<uint8>& OutEncryptedBytes)
	{
		DATA_BLOB In;
		In.pbData = const_cast<BYTE*>(PlainBytes.GetData());
		In.cbData = static_cast<DWORD>(PlainBytes.Num());

		DATA_BLOB Out;
		FMemory::Memzero(Out);

		const BOOL bOk = CryptProtectData(&In, L"DT4MOB Ditto credentials", nullptr, nullptr, nullptr,
			CRYPTPROTECT_UI_FORBIDDEN, &Out);

		if (!bOk)
		{
			UE_LOG(LogTemp, Error, TEXT("CredentialStoreService: CryptProtectData failed (error %u)"), GetLastError());
			return false;
		}

		OutEncryptedBytes.SetNumUninitialized(Out.cbData);
		FMemory::Memcpy(OutEncryptedBytes.GetData(), Out.pbData, Out.cbData);
		LocalFree(Out.pbData);
		return true;
	}

	/**
	 * @brief Decrypts a blob produced by DpapiEncrypt().
	 *
	 * @param EncryptedBytes  Blob read from the credential file.
	 * @param OutPlainBytes   Receives the decrypted data on success.
	 * @return                True on success; false (with a warning logged) when the blob was
	 *                        written by another user or machine, or is corrupted.
	 */
	bool DpapiDecrypt(const TArray<uint8>& EncryptedBytes, TArray<uint8>& OutPlainBytes)
	{
		DATA_BLOB In;
		In.pbData = const_cast<BYTE*>(EncryptedBytes.GetData());
		In.cbData = static_cast<DWORD>(EncryptedBytes.Num());

		DATA_BLOB Out;
		FMemory::Memzero(Out);

		const BOOL bOk = CryptUnprotectData(&In, nullptr, nullptr, nullptr, nullptr,
			CRYPTPROTECT_UI_FORBIDDEN, &Out);

		if (!bOk)
		{
			UE_LOG(LogTemp, Warning, TEXT("CredentialStoreService: CryptUnprotectData failed (error %u) — stored credentials are unreadable (different user/machine, or corrupted)"), GetLastError());
			return false;
		}

		OutPlainBytes.SetNumUninitialized(Out.cbData);
		FMemory::Memcpy(OutPlainBytes.GetData(), Out.pbData, Out.cbData);
		LocalFree(Out.pbData);
		return true;
	}

	/**
	 * @brief Returns the path of the encrypted credential file.
	 * @return FPaths::ProjectSavedDir() / "Credentials.dat".
	 */
	FString CredentialFilePath()
	{
		return FPaths::ProjectSavedDir() / TEXT("Credentials.dat");
	}

	/**
	 * @brief Checks for a stored secret (Windows: the credential file exists).
	 * @return True if the credential file exists.
	 */
	bool PlatformHasSecret()
	{
		return FPlatformFileManager::Get().GetPlatformFile().FileExists(*CredentialFilePath());
	}

	/**
	 * @brief Encrypts the secret with DPAPI and writes it to the credential file, replacing any
	 *        previous file.
	 * @param PlainBytes  UTF-8 plaintext to store.
	 * @return            True on success; false if encryption or the file write fails.
	 */
	bool PlatformStoreSecret(const TArray<uint8>& PlainBytes)
	{
		TArray<uint8> EncryptedBytes;
		if (!DpapiEncrypt(PlainBytes, EncryptedBytes))
			return false;

		if (!FFileHelper::SaveArrayToFile(EncryptedBytes, *CredentialFilePath()))
		{
			UE_LOG(LogTemp, Error, TEXT("CredentialStoreService: failed to write credential file to %s"), *CredentialFilePath());
			return false;
		}
		return true;
	}

	/**
	 * @brief Reads the credential file and decrypts it with DPAPI.
	 * @param OutPlainBytes  Receives the UTF-8 plaintext on success.
	 * @return               False if the file is missing, unreadable, or cannot be decrypted.
	 */
	bool PlatformLoadSecret(TArray<uint8>& OutPlainBytes)
	{
		if (!PlatformHasSecret())
			return false;

		TArray<uint8> EncryptedBytes;
		if (!FFileHelper::LoadFileToArray(EncryptedBytes, *CredentialFilePath()))
		{
			UE_LOG(LogTemp, Warning, TEXT("CredentialStoreService: failed to read credential file"));
			return false;
		}

		return DpapiDecrypt(EncryptedBytes, OutPlainBytes);
	}

	/** @brief Deletes the credential file if it exists. */
	void PlatformDeleteSecret()
	{
		if (PlatformHasSecret())
			FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*CredentialFilePath());
	}

#elif PLATFORM_MAC

	/**
	 * @brief Builds the Keychain query that identifies the single credential item.
	 *
	 * The same service/account pair ("DT4MOB" / "DittoCredentials") is used by store, load,
	 * delete and has, so they always address the same generic password item.
	 *
	 * @return New mutable dictionary owned by the caller (release with CFRelease).
	 */
	CFMutableDictionaryRef MakeBaseQuery()
	{
		CFMutableDictionaryRef Query = CFDictionaryCreateMutable(
			kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

		CFDictionaryAddValue(Query, kSecClass, kSecClassGenericPassword);
		CFDictionaryAddValue(Query, kSecAttrService, CFSTR("DT4MOB"));
		CFDictionaryAddValue(Query, kSecAttrAccount, CFSTR("DittoCredentials"));
		return Query;
	}

	/**
	 * @brief Checks for a stored secret (macOS: the Keychain item exists).
	 * @return True if SecItemCopyMatching finds the item.
	 */
	bool PlatformHasSecret()
	{
		CFMutableDictionaryRef Query = MakeBaseQuery();
		CFDictionaryAddValue(Query, kSecReturnData, kCFBooleanFalse);
		CFDictionaryAddValue(Query, kSecMatchLimit, kSecMatchLimitOne);

		CFTypeRef Result = nullptr;
		const OSStatus Status = SecItemCopyMatching(Query, &Result);
		if (Result) CFRelease(Result);
		CFRelease(Query);
		return Status == errSecSuccess;
	}

	/**
	 * @brief Stores the secret as the Keychain item, replacing any existing one.
	 *
	 * The item is accessible after the first unlock following a reboot
	 * (kSecAttrAccessibleAfterFirstUnlock).
	 *
	 * @param PlainBytes  UTF-8 plaintext to store; the Keychain encrypts it at rest.
	 * @return            True on success; false (with an error logged) if SecItemAdd fails.
	 */
	bool PlatformStoreSecret(const TArray<uint8>& PlainBytes)
	{
		// Delete any existing item first. Simpler than SecItemUpdate and just as safe here,
		// since this is a full overwrite of a single-item credential store.
		CFMutableDictionaryRef DeleteQuery = MakeBaseQuery();
		SecItemDelete(DeleteQuery);
		CFRelease(DeleteQuery);

		CFDataRef DataRef = CFDataCreate(kCFAllocatorDefault, PlainBytes.GetData(), PlainBytes.Num());

		CFMutableDictionaryRef AddQuery = MakeBaseQuery();
		CFDictionaryAddValue(AddQuery, kSecValueData, DataRef);
		CFDictionaryAddValue(AddQuery, kSecAttrAccessible, kSecAttrAccessibleAfterFirstUnlock);

		const OSStatus Status = SecItemAdd(AddQuery, nullptr);
		CFRelease(DataRef);
		CFRelease(AddQuery);

		if (Status != errSecSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("CredentialStoreService: SecItemAdd failed (OSStatus %d)"), static_cast<int32>(Status));
			return false;
		}
		return true;
	}

	/**
	 * @brief Reads the secret from the Keychain item.
	 * @param OutPlainBytes  Receives the stored UTF-8 plaintext on success.
	 * @return               False (with a warning logged) if the item is missing or unreadable.
	 */
	bool PlatformLoadSecret(TArray<uint8>& OutPlainBytes)
	{
		CFMutableDictionaryRef Query = MakeBaseQuery();
		CFDictionaryAddValue(Query, kSecReturnData, kCFBooleanTrue);
		CFDictionaryAddValue(Query, kSecMatchLimit, kSecMatchLimitOne);

		CFTypeRef Result = nullptr;
		const OSStatus Status = SecItemCopyMatching(Query, &Result);
		CFRelease(Query);

		if (Status != errSecSuccess || !Result)
		{
			UE_LOG(LogTemp, Warning, TEXT("CredentialStoreService: SecItemCopyMatching failed (OSStatus %d)"), static_cast<int32>(Status));
			return false;
		}

		CFDataRef DataRef = static_cast<CFDataRef>(Result);
		OutPlainBytes.SetNumUninitialized(CFDataGetLength(DataRef));
		FMemory::Memcpy(OutPlainBytes.GetData(), CFDataGetBytePtr(DataRef), OutPlainBytes.Num());
		CFRelease(Result);
		return true;
	}

	/** @brief Deletes the Keychain item; does nothing if it does not exist. */
	void PlatformDeleteSecret()
	{
		CFMutableDictionaryRef Query = MakeBaseQuery();
		SecItemDelete(Query);
		CFRelease(Query);
	}

#else

	// Platforms without a secure storage backend: nothing is ever stored or found.

	/**
	 * @brief Stub for platforms without a backend.
	 * @return Always false.
	 */
	bool PlatformHasSecret() { return false; }

	/**
	 * @brief Stub for platforms without a backend; stores nothing.
	 * @return Always false, which makes StoreCredentials() log a warning.
	 */
	bool PlatformStoreSecret(const TArray<uint8>&) { return false; }

	/**
	 * @brief Stub for platforms without a backend.
	 * @return Always false, so TryLoadCredentials() fails and the login screen is shown.
	 */
	bool PlatformLoadSecret(TArray<uint8>&) { return false; }

	/** @brief Stub for platforms without a backend; does nothing. */
	void PlatformDeleteSecret() {}

#endif
}

bool UCredentialStoreService::HasStoredCredentials() const
{
	return PlatformHasSecret();
}

void UCredentialStoreService::StoreCredentials(const FString& Host, const FString& Username, const FString& Password)
{
	const FString Plain = Host + FieldDelimiter + Username + FieldDelimiter + Password;

	TArray<uint8> PlainBytes;
	FTCHARToUTF8 Convert(*Plain);
	PlainBytes.Append(reinterpret_cast<const uint8*>(Convert.Get()), Convert.Length());

	if (PlatformStoreSecret(PlainBytes))
	{
		UE_LOG(LogTemp, Log, TEXT("CredentialStoreService: credentials stored for user '%s'"), *Username);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("CredentialStoreService: no secure-storage backend available on this platform, credentials will not persist across launches"));
	}
}

bool UCredentialStoreService::TryLoadCredentials(FString& OutHost, FString& OutUsername, FString& OutPassword) const
{
	TArray<uint8> PlainBytes;
	if (!PlatformLoadSecret(PlainBytes))
		return false;

	PlainBytes.Add(0); // null-terminate for FUTF8ToTCHAR
	const FString Plain = FUTF8ToTCHAR(reinterpret_cast<const ANSICHAR*>(PlainBytes.GetData())).Get();

	TArray<FString> Fields;
	Plain.ParseIntoArray(Fields, FieldDelimiter, /*InCullEmpty=*/false);

	if (Fields.Num() != 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("CredentialStoreService: stored credential data is malformed"));
		return false;
	}

	OutHost = Fields[0];
	OutUsername = Fields[1];
	OutPassword = Fields[2];
	return true;
}

void UCredentialStoreService::ClearCredentials()
{
	PlatformDeleteSecret();
	UE_LOG(LogTemp, Log, TEXT("CredentialStoreService: stored credentials cleared"));
}

FString UCredentialStoreService::GetCredentialFilePath() const
{
	// Retained for API compatibility and not called. Windows uses its own CredentialFilePath()
	// helper above; macOS has no file because the Keychain is the store.
	return FPaths::ProjectSavedDir() / TEXT("Credentials.dat");
}
