/**
 * @file
 * @brief Declares UDittoRestClient, the Ditto REST and Keycloak authentication implementation of IBackendRestClient.
 * @ingroup backend
 */
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
 * @ingroup backend
 */
UCLASS()
class UDittoRestClient : public UObject, public IBackendRestClient
{
	GENERATED_BODY()

public:
	/**
	 * @brief Must be called once, immediately after construction, before any other method.
	 * @param bInUseHttps         True to use https for REST calls.
	 * @param bInUseOAuth         True for Keycloak OAuth2, false for HTTP Basic auth.
	 * @param InOAuthClientId     OAuth2 client_id sent to Keycloak.
	 * @param InOnAuthHeaderReady Called with the new Authorization header each time one becomes available.
	 */
	void Configure(bool bInUseHttps, bool bInUseOAuth, const FString& InOAuthClientId,
		TFunction<void(const FString&)> InOnAuthHeaderReady);

	/** @brief Clears the token refresh timer and drops the HTTP module pointer. */
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
	/**
	 * @brief Requests a token with the OAuth2 password grant, using the stored credentials.
	 * @param OnComplete Optional callback with the result.
	 */
	void GetOAuthToken(TFunction<void(bool)> OnComplete = nullptr);

	/** @brief Renews the token with the refresh token, or with a full password grant if there is none. */
	void RefreshOAuthToken();

	/**
	 * @brief POSTs a grant to the Keycloak token endpoint (/auth/realms/dt4mob/protocol/openid-connect/token).
	 *
	 * On success stores the access and refresh tokens, schedules a refresh 30 s before expiry,
	 * reports the new header through OnAuthHeaderReady and sends any queued requests. Does
	 * nothing if a token request is already running.
	 *
	 * @param Body       Form-encoded grant body.
	 * @param OnComplete Optional callback with the result.
	 */
	void SendTokenRequest(const FString& Body, TFunction<void(bool)> OnComplete = nullptr);

	/** @brief Sends every request queued while the token was not ready. */
	void FlushPendingRequests();

	/**
	 * @brief Sends a request now if authenticated, otherwise queues it and starts authentication.
	 * @param Request Prepared request without the Authorization header.
	 */
	void SendAuthenticatedRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request);

	/**
	 * @brief Sets the Authorization header on a request.
	 * @param Request Request to modify.
	 */
	void SetCommonHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request);

	FString Username;      ///< User name from Login(). Cleared by Logout().
	FString Password;      ///< Password from Login(), kept in memory to renew tokens. Cleared by Logout().
	FString BaseUrl;       ///< Scheme plus host, e.g. "https://" followed by Host.
	FString Host;          ///< Host from Login(), without scheme.
	FString OAuthClientId; ///< OAuth2 client_id from Configure().

	bool bUseOAuth = true; ///< True for Keycloak OAuth2, false for HTTP Basic.
	bool bUseHttps = true; ///< True to build https URLs.

	FHttpModule* Http = nullptr; ///< Engine HTTP module, set in Configure().

	FString OAuthToken;    ///< Current access token. Empty when not authenticated.
	FString RefreshToken;  ///< Refresh token from the last token response.

	/** True once Login() has been called with credentials (cleared by Logout()). */
	bool bCredentialsSet = false;

	/** True while a token HTTP request is in-flight: prevents duplicate requests. */
	bool bAuthInProgress = false;

	/** API requests that arrived before the token was ready, flushed on token acquisition. */
	TArray<TFunction<void()>> PendingRequests;

	/** Timer that fires RefreshOAuthToken() ~30 s before the current token expires. */
	FTimerHandle TokenRefreshTimer;

	/** Invoked whenever a valid Authorization header becomes available; UBackendConnectionService
	 *  re-broadcasts this as its own OnAuthHeaderReady delegate. */
	TFunction<void(const FString&)> OnAuthHeaderReady;
};
