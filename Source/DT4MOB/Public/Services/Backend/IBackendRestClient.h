/**
 * @file
 * @brief Declares IBackendRestClient, the interface for backend authentication and REST access to things.
 * @ingroup backend
 */
#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * @brief Abstract contract for the REST/HTTP half of talking to a digital-twin backend:
 *        authentication and CRUD access to "things".
 *
 * UBackendConnectionService owns exactly one implementation of this (UDittoRestClient) and forwards
 * every one of its own public methods to it. A second backend means writing a second
 * implementation and constructing it instead, in UBackendConnectionService::Initialize: no change to
 * any of UBackendConnectionService's own callers, since its public signatures don't move.
 * @ingroup backend
 */
class DT4MOB_API IBackendRestClient
{
public:
	virtual ~IBackendRestClient() = default;

	/** @brief Releases any timers/handles tied to the current World. Called from Deinitialize(). */
	virtual void Shutdown() = 0;

	/**
	 * @brief Stores the credentials and authenticates against the backend.
	 * @param InHost     Backend host name without scheme.
	 * @param InUsername User name.
	 * @param InPassword Password.
	 * @param OnComplete Called with true once an Authorization header is available, false if authentication failed.
	 */
	virtual void Login(const FString& InHost, const FString& InUsername, const FString& InPassword,
		TFunction<void(bool bSuccess)> OnComplete) = 0;

	/** @brief Forgets the credentials and tokens and drops any queued requests. */
	virtual void Logout() = 0;

	/**
	 * @brief Checks whether requests can be sent.
	 * @return True once a usable Authorization header exists.
	 */
	virtual bool IsAuthenticated() const = 0;

	/**
	 * @brief Builds the current Authorization header value.
	 * @return "Bearer " plus the access token, "Basic " plus base64(user:password), or an empty string when no token is held yet.
	 */
	virtual FString GetCurrentAuthHeader() const = 0;

	/**
	 * @brief Returns the host passed to Login().
	 * @return Host name without scheme, or empty before login.
	 */
	virtual FString GetHost() const = 0;

	/**
	 * @brief Fetches every thing, page by page.
	 * @param OnPageReceived Called once per page with that page's things.
	 * @param OnCompleted    Called once after the last page, or after an error.
	 * @param Filter         Optional backend search filter. Empty means no filter.
	 */
	virtual void GetAllThings(
		TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
		TFunction<void()> OnCompleted,
		const FString& Filter) = 0;

	/**
	 * @brief Fetches the things whose attributes/geotile lies in [Lower, Upper), page by page.
	 * Things whose expiry time has already passed are left out.
	 * @param Lower          Inclusive lower bound of the geotile range.
	 * @param Upper          Exclusive upper bound of the geotile range.
	 * @param OnPageReceived Called once per page with that page's things.
	 * @param OnCompleted    Called once after the last page, or after an error.
	 */
	virtual void GetThingsByGeotileBounds(
		int64 Lower,
		int64 Upper,
		TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
		TFunction<void()> OnCompleted) = 0;

	/**
	 * @brief Fetches one thing.
	 * @param ThingId    Id of the thing.
	 * @param OnComplete Called with the thing JSON, or a null pointer on failure.
	 */
	virtual void GetThingById(
		const FString& ThingId,
		TFunction<void(TSharedPtr<FJsonObject>)> OnComplete) = 0;

	/**
	 * @brief Creates or replaces a thing.
	 * @param ThingId    Id of the thing.
	 * @param Body       Full thing JSON to store.
	 * @param OnComplete Called with true if the backend accepted the request.
	 */
	virtual void PutThing(
		const FString& ThingId,
		TSharedPtr<FJsonObject> Body,
		TFunction<void(bool bSuccess)> OnComplete) = 0;
};
