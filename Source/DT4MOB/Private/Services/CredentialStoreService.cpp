// Fill out your copyright notice in the Description page of Project Settings.

/** @file CredentialStoreService.cpp
 *  @brief Implementation of UCredentialStoreService. All logic documentation is in the header.
 *
 * Each platform gets its own secret-storage backend behind a common
 * PlatformStoreSecret/PlatformLoadSecret/PlatformDeleteSecret/PlatformHasSecret interface:
 *
 *  - Windows: DPAPI (CryptProtectData/CryptUnprotectData), tied to the logged-in Windows user.
 *    Encrypts a byte blob and writes it to a file under Saved/.
 *  - macOS: Keychain Services (SecItemAdd/SecItemCopyMatching/SecItemDelete). No file involved —
 *    the OS keychain IS the storage, and encrypts at rest itself.
 *  - Any other platform (including Linux): no backend implemented — TryLoadCredentials() always
 *    fails and StoreCredentials() is a no-op, so the login screen is simply shown every launch.
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
	// Fields are newline-separated; Host/Username/Password aren't expected to contain '\n' in practice.
	const TCHAR* FieldDelimiter = TEXT("\n");

#if PLATFORM_WINDOWS

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

	FString CredentialFilePath()
	{
		return FPaths::ProjectSavedDir() / TEXT("Credentials.dat");
	}

	bool PlatformHasSecret()
	{
		return FPlatformFileManager::Get().GetPlatformFile().FileExists(*CredentialFilePath());
	}

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

	void PlatformDeleteSecret()
	{
		if (PlatformHasSecret())
			FPlatformFileManager::Get().GetPlatformFile().DeleteFile(*CredentialFilePath());
	}

#elif PLATFORM_MAC

	// Same service/account pair identifies a single keychain item across store/load/delete/has.
	CFMutableDictionaryRef MakeBaseQuery()
	{
		CFMutableDictionaryRef Query = CFDictionaryCreateMutable(
			kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

		CFDictionaryAddValue(Query, kSecClass, kSecClassGenericPassword);
		CFDictionaryAddValue(Query, kSecAttrService, CFSTR("DT4MOB"));
		CFDictionaryAddValue(Query, kSecAttrAccount, CFSTR("DittoCredentials"));
		return Query;
	}

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

	bool PlatformStoreSecret(const TArray<uint8>& PlainBytes)
	{
		// Delete any existing item first — simpler and just as safe as SecItemUpdate here,
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

	void PlatformDeleteSecret()
	{
		CFMutableDictionaryRef Query = MakeBaseQuery();
		SecItemDelete(Query);
		CFRelease(Query);
	}

#else

	bool PlatformHasSecret() { return false; }
	bool PlatformStoreSecret(const TArray<uint8>&) { return false; }
	bool PlatformLoadSecret(TArray<uint8>&) { return false; }
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
	// Retained for API compatibility; Windows uses this internally via its own
	// CredentialFilePath() helper above. macOS has no file — Keychain is the store.
	return FPaths::ProjectSavedDir() / TEXT("Credentials.dat");
}
