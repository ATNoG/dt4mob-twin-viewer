// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Http.h"
#include "DittoService.generated.h"

/**
 * @brief GameInstance subsystem that provides HTTP access to the Ditto digital-twin REST API.
 *
 * Currently exposes a single paginated query (GetAllThings()) that streams all known
 * Ditto things to the caller one page at a time via the OnPageReceived callback.
 *
 * Authentication uses HTTP Basic auth with the configured Username/Password credentials.
 *
 * @note Credentials are stored in plain text as UPROPERTY strings — consider moving
 *       them to a secure configuration source for production builds.
 */
UCLASS()
class DT4MOB_API UDittoService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @brief Initialises the HTTP module reference.
	 * @param Collection Subsystem collection provided by the engine.
	 */
	virtual void Initialize(FSubsystemCollectionBase &Collection) override;

	/**
	 * @brief Asynchronously fetches all Ditto things matching the configured filter.
	 *
	 * Fires OnPageReceived for each page of results (up to 200 items per page).
	 * When all pages have been retrieved (or pagination is exhausted), OnCompleted is called.
	 *
	 * @param OnPageReceived  Callback invoked with the array of parsed JSON objects for each page.
	 * @param OnCompleted     Callback invoked once when the fetch is fully complete or has failed.
	 */
	void GetAllThings(
		TFunction<void(const TArray<TSharedPtr<FJsonObject>> &)> OnPageReceived,
		TFunction<void()> OnCompleted);

private:
	/** @brief Ditto API username used for Basic authentication. */
	FString Username;

	/** @brief Ditto API password used for Basic authentication. */
	FString Password;

	/** @brief Cached pointer to the FHttpModule singleton. */
	FHttpModule *Http = nullptr;

	/** @brief Base URL of the Ditto REST API (without trailing slash). */
	FString BaseUrl;

	/**
	 * @brief Adds the Authorization header to the given request.
	 *
	 * Uses HTTP Basic auth encoded from Username:Password.
	 *
	 * @param Request The HTTP request to annotate.
	 */
	void SetCommonHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request);
};
