// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Http.h"
#include "DittoService.generated.h"

/**
 * @brief GameInstance subsystem that provides HTTP access to the Ditto digital-twin REST API.
 *
 * Authentication uses OAuth2 password grant against a Keycloak endpoint.  A token is fetched
 * immediately on Initialize(); any API calls that arrive before the token is ready are queued
 * and flushed automatically once it lands.  The token is proactively refreshed ~30 s before
 * expiry using the refresh_token returned by Keycloak.
 */
UCLASS()
class DT4MOB_API UDittoService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

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
		TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
		TFunction<void()> OnCompleted);

	/**
	 * @brief Fetches Ditto things whose geotile attribute falls within the tile that contains
	 *        the given geographic point at the specified zoom level.
	 *
	 * @param Lat             Latitude of the camera / viewport centre in decimal degrees.
	 * @param Lng             Longitude of the camera / viewport centre in decimal degrees.
	 * @param TileZoom        Zoom level that controls the tile size (higher = smaller tile).
	 * @param OnPageReceived  Callback invoked with each page of matching JSON objects.
	 * @param OnCompleted     Callback invoked once when the fetch is fully done or has failed.
	 */
	void GetThingsByGeotile(
		double Lat,
		double Lng,
		int32 TileZoom,
		TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
		TFunction<void()> OnCompleted);

	/**
	 * @brief Fetches a single Ditto thing by its full identifier.
	 *
	 * @param ThingId    Full Ditto thing identifier (e.g. "traci:vehicle-42").
	 * @param OnComplete Callback with the thing JSON, or nullptr on failure.
	 */
	void GetThingById(
		const FString& ThingId,
		TFunction<void(TSharedPtr<FJsonObject>)> OnComplete);

	/**
	 * @brief Creates or replaces a Ditto thing via HTTP PUT.
	 *
	 * @param ThingId    Full Ditto thing identifier (e.g. "ignition-point:<guid>").
	 * @param Body       JSON object to send as the request body.
	 * @param OnComplete Callback with success flag.
	 */
	void PutThing(
		const FString& ThingId,
		TSharedPtr<FJsonObject> Body,
		TFunction<void(bool bSuccess)> OnComplete);

	/**
	 * @brief Computes the OSM quad-tile integer key for a geographic point at the given zoom level.
	 *
	 * @param Lat   Latitude in decimal degrees.
	 * @param Lng   Longitude in decimal degrees.
	 * @param Zoom  Zoom level (0-31).
	 * @return 64-bit quadkey integer.
	 */
	static int64 GetQuadkey(double Lat, double Lng, int32 Zoom);

	/**
	 * @brief Returns the inclusive geotile range [OutLower, OutUpper) for the tile containing the point.
	 *
	 * @param Lat       Latitude in decimal degrees.
	 * @param Lng       Longitude in decimal degrees.
	 * @param TileZoom  Zoom level at which to compute the tile.
	 * @param OutLower  Inclusive lower bound.
	 * @param OutUpper  Exclusive upper bound.
	 * @param MaxZoom   Zoom level at which geotiles are stored in Ditto (default 31).
	 */
	static void GetTileBounds(double Lat, double Lng, int32 TileZoom, int64& OutLower, int64& OutUpper, int32 MaxZoom = 31);

	/**
	 * @brief Maps a camera altitude above the WGS-84 ellipsoid to a quadtile zoom level.
	 *
	 * @param AltitudeMeters  Camera altitude in metres.
	 * @return Zoom level in [0, MaxTileZoom].
	 */
	static int32 AltitudeToZoomLevel(double AltitudeMeters);

	static constexpr int32 MaxTileZoom = 20;

private:
	/**
	 * @brief Kicks off an OAuth2 password-grant token request.  Called once at startup.
	 */
	void GetOAuthToken();

	/**
	 * @brief Kicks off an OAuth2 refresh-token request.  Called automatically before token expiry.
	 *        Falls back to a full re-authentication if no refresh token is available.
	 */
	void RefreshOAuthToken();

	/**
	 * @brief Shared implementation for both initial auth and refresh.
	 *
	 * POSTs the given URL-encoded body to the token endpoint, parses the response,
	 * stores access_token / refresh_token, schedules the next refresh, and flushes
	 * any queued API requests.
	 *
	 * @param Body       URL-encoded POST body (differs between password and refresh grants).
	 * @param OnComplete Optional callback invoked with true on success, false on failure.
	 */
	void SendTokenRequest(const FString& Body, TFunction<void(bool)> OnComplete = nullptr);

	/**
	 * @brief Drains the pending-request queue after a token has been obtained.
	 */
	void FlushPendingRequests();

	/**
	 * @brief Fires an authenticated HTTP request, gating it behind token availability.
	 *
	 * If the token is ready the request is dispatched immediately.  Otherwise it is
	 * pushed onto PendingRequests (and a token fetch is started if one is not already
	 * in flight).  The request must have its URL, verb, headers, and completion handler
	 * fully configured before calling this.
	 *
	 * @param Request  The fully-configured request to dispatch.
	 */
	void SendAuthenticatedRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request);

	/**
	 * @brief Adds the Authorization Bearer header to the given request.
	 */
	void SetCommonHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request);

	FString Username;
	FString Password;
	FString BaseUrl;

	/** When false, falls back to HTTP Basic auth (Base64 username:password). Controlled by Secrets.ini [Ditto] UseOAuth. */
	bool bUseOAuth = true;

	FHttpModule* Http = nullptr;

	FString OAuthToken;
	FString RefreshToken;

	/** True while a token HTTP request is in-flight — prevents duplicate requests. */
	bool bAuthInProgress = false;

	/** API requests that arrived before the token was ready, flushed on token acquisition. */
	TArray<TFunction<void()>> PendingRequests;

	/** Timer that fires RefreshOAuthToken() ~30 s before the current token expires. */
	FTimerHandle TokenRefreshTimer;
};
