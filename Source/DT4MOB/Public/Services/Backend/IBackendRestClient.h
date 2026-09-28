#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

/**
 * @brief Abstract contract for the REST/HTTP half of talking to a digital-twin backend:
 *        authentication and CRUD access to "things".
 *
 * UBackendConnectionService owns exactly one implementation of this (UDittoRestClient) and forwards
 * every one of its own public methods to it. A second backend means writing a second
 * implementation and constructing it instead, in UBackendConnectionService::Initialize — no change to
 * any of UBackendConnectionService's own callers, since its public signatures don't move.
 */
class DT4MOB_API IBackendRestClient
{
public:
	virtual ~IBackendRestClient() = default;

	/** @brief Releases any timers/handles tied to the current World. Called from Deinitialize(). */
	virtual void Shutdown() = 0;

	virtual void Login(const FString& InHost, const FString& InUsername, const FString& InPassword,
		TFunction<void(bool bSuccess)> OnComplete) = 0;
	virtual void Logout() = 0;
	virtual bool IsAuthenticated() const = 0;
	virtual FString GetCurrentAuthHeader() const = 0;
	virtual FString GetHost() const = 0;

	virtual void GetAllThings(
		TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
		TFunction<void()> OnCompleted,
		const FString& Filter) = 0;

	virtual void GetThingsByGeotileBounds(
		int64 Lower,
		int64 Upper,
		TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
		TFunction<void()> OnCompleted) = 0;

	virtual void GetThingById(
		const FString& ThingId,
		TFunction<void(TSharedPtr<FJsonObject>)> OnComplete) = 0;

	virtual void PutThing(
		const FString& ThingId,
		TSharedPtr<FJsonObject> Body,
		TFunction<void(bool bSuccess)> OnComplete) = 0;
};
