#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Services/Backend/IBackendRestClient.h"
#include "Http.h"
#include "DittoRestClient.generated.h"

/**
 * @brief IBackendRestClient implementation for Eclipse Ditto: OAuth2 password grant against
 *        a Keycloak endpoint (or HTTP Basic as a fallback), and Ditto's /api/2/... REST
 *        surface. This is the only place the Ditto/Keycloak-specific request shapes (token
 *        endpoint, grant bodies, search/get/put URLs) exist.
 *
 * A plain UObject rather than its own GameInstanceSubsystem: it is owned and constructed by
 * UBackendConnectionService (via NewObject<UDittoRestClient>(this) in Initialize), which keeps it alive
 * through a UPROPERTY and gets a valid GetWorld() through the normal Outer-chain resolution.
 */
UCLASS()
class UDittoRestClient : public UObject, public IBackendRestClient
{
	GENERATED_BODY()

public:
	/** @brief Must be called once, immediately after construction, before any other method. */
	void Configure(bool bInUseHttps, bool bInUseOAuth, const FString& InOAuthClientId,
		TFunction<void(const FString&)> InOnAuthHeaderReady);

	virtual void Shutdown() override;

	virtual void Login(const FString& InHost, const FString& InUsername, const FString& InPassword,
		TFunction<void(bool bSuccess)> OnComplete) override;
	virtual void Logout() override;
	virtual bool IsAuthenticated() const override;
	virtual FString GetCurrentAuthHeader() const override;
	virtual FString GetHost() const override { return Host; }

	virtual void GetAllThings(
		TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
		TFunction<void()> OnCompleted,
		const FString& Filter) override;

	virtual void GetThingsByGeotileBounds(
		int64 Lower,
		int64 Upper,
		TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
		TFunction<void()> OnCompleted) override;

	virtual void GetThingById(
		const FString& ThingId,
		TFunction<void(TSharedPtr<FJsonObject>)> OnComplete) override;

	virtual void PutThing(
		const FString& ThingId,
		TSharedPtr<FJsonObject> Body,
		TFunction<void(bool bSuccess)> OnComplete) override;

private:
	void GetOAuthToken(TFunction<void(bool)> OnComplete = nullptr);
	void RefreshOAuthToken();
	void SendTokenRequest(const FString& Body, TFunction<void(bool)> OnComplete = nullptr);
	void FlushPendingRequests();
	void SendAuthenticatedRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request);
	void SetCommonHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request);

	FString Username;
	FString Password;
	FString BaseUrl;
	FString Host;
	FString OAuthClientId;

	bool bUseOAuth = true;
	bool bUseHttps = true;

	FHttpModule* Http = nullptr;

	FString OAuthToken;
	FString RefreshToken;

	/** True once Login() has been called with credentials (cleared by Logout()). */
	bool bCredentialsSet = false;

	/** True while a token HTTP request is in-flight — prevents duplicate requests. */
	bool bAuthInProgress = false;

	/** API requests that arrived before the token was ready, flushed on token acquisition. */
	TArray<TFunction<void()>> PendingRequests;

	/** Timer that fires RefreshOAuthToken() ~30 s before the current token expires. */
	FTimerHandle TokenRefreshTimer;

	/** Invoked whenever a valid Authorization header becomes available; UBackendConnectionService
	 *  re-broadcasts this as its own OnAuthHeaderReady delegate. */
	TFunction<void(const FString&)> OnAuthHeaderReady;
};
