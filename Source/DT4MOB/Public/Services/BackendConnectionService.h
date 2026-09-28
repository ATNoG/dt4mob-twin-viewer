// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BackendConnectionService.generated.h"

class IBackendRestClient;
class FJsonObject;

/**
 * @brief GameInstance subsystem that owns the digital-twin backend connection: login, the
 *        current auth header, and REST access to things.
 *
 * The actual protocol (currently Eclipse Ditto: OAuth2 password grant against a Keycloak
 * endpoint, or HTTP Basic as a fallback, and Ditto's /api/2/... REST surface) lives behind
 * IBackendRestClient, not in this class — see UDittoRestClient. This class owns lifecycle,
 * delegates, the config read from the DA_DittoSecrets DataAsset, and the geotile/quadkey math,
 * and forwards everything else to its one IBackendRestClient implementation.
 *
 * Unlike a design-time secrets asset, credentials now arrive at runtime via Login() (called by
 * the login screen, either with user-entered values or with values restored from
 * UCredentialStoreService). Initialize() no longer auto-authenticates — it only pulls
 * non-credential defaults (bUseHttps/bUseOAuth/OAuthClientId/WsStartMessage/a default Host) from
 * the DA_DittoSecrets DataAsset if present, for convenience/dev use.
 *
 * Any API calls that arrive before Login() succeeds are queued and flushed automatically once a
 * token lands. The token is proactively refreshed ~30 s before expiry using the refresh_token
 * returned by Keycloak. The refresh/access tokens are never persisted to disk — only
 * Username/Password are (encrypted, via UCredentialStoreService), so a fresh password grant is
 * performed on every launch instead of trying to reuse a token that may have gone stale.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBackendAuthHeaderReady, const FString&, AuthHeader);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBackendLoggedOut);

UCLASS()
class DT4MOB_API UBackendConnectionService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * @brief Broadcast whenever a valid Authorization header is available: on initial token
	 *        acquisition (OAuth) or immediately at Initialize() (Basic). Re-broadcast on refresh.
	 *        WSService listens here to defer its connect until auth is ready.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnBackendAuthHeaderReady OnAuthHeaderReady;

	/** @brief Broadcast whenever Logout() is called, so the app can return to the login screen. */
	UPROPERTY(BlueprintAssignable)
	FOnBackendLoggedOut OnLoggedOut;

	/**
	 * @brief Authenticates with the given credentials (login screen entry point).
	 *
	 * Sets Host/Username/Password, rebuilds BaseUrl, and performs an OAuth2 password grant
	 * (or, if bUseOAuth is false, makes Basic auth immediately available). Safe to call again
	 * to re-authenticate with different credentials (e.g. retry after a failed login).
	 *
	 * @param InHost      Ditto host, no scheme (e.g. "your-ditto-host.example.com").
	 * @param InUsername  Ditto/Keycloak username.
	 * @param InPassword  Ditto/Keycloak password.
	 * @param OnComplete  Invoked with true once a valid auth header is available, false on failure.
	 */
	void Login(const FString& InHost, const FString& InUsername, const FString& InPassword,
		TFunction<void(bool bSuccess)> OnComplete);

	/**
	 * @brief Clears all in-memory credentials and tokens and cancels the refresh timer.
	 *
	 * Does NOT touch anything on disk — callers are expected to also clear
	 * UCredentialStoreService separately. Broadcasts OnLoggedOut.
	 */
	void Logout();

	/** @brief True once Login() has produced a usable Authorization header (Bearer or Basic). */
	bool IsAuthenticated() const;

	/**
	 * @brief Returns the current Authorization header value, or an empty string if not yet ready.
	 *        Returns "Bearer <token>" when OAuth is active, "Basic <base64>" otherwise.
	 */
	FString GetCurrentAuthHeader() const;

	/** Ditto host currently in use (empty until Login() is called). */
	FString GetHost() const;

	/** Default Host pulled from the DA_DittoSecrets DataAsset, if any — used to prefill the login form. */
	FString GetDefaultHost() const { return DefaultHost; }

	/** Whether the Ditto REST/WS endpoints use TLS, as configured on the secrets DataAsset. */
	bool IsUseHttps() const { return bUseHttps; }

	/** WebSocket START-SEND-EVENTS message, as configured on the secrets DataAsset. */
	FString GetWsStartMessage() const { return WsStartMessage; }

	/**
	 * @brief Asynchronously fetches all Ditto things matching the configured filter.
	 *
	 * Fires OnPageReceived for each page of results (up to 200 items per page).
	 * When all pages have been retrieved (or pagination is exhausted), OnCompleted is called.
	 *
	 * @param OnPageReceived  Callback invoked with the array of parsed JSON objects for each page.
	 * @param OnCompleted     Callback invoked once when the fetch is fully complete or has failed.
	 * @param Filter          Optional Ditto search filter (e.g. `like(thingId,"*sinalizacao*")`).
	 *                        Empty means no filter — every thing is returned.
	 */
	void GetAllThings(
		TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
		TFunction<void()> OnCompleted,
		const FString& Filter = FString());

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

	/** Converts a geographic point to OSM tile X/Y grid coordinates at the given zoom. */
	static void GetTileXY(double Lat, double Lng, int32 Zoom, int64& OutX, int64& OutY);

	/** Converts tile grid X/Y coordinates back to a quadkey integer. */
	static int64 GetQuadkeyFromXY(int64 X, int64 Y, int32 Zoom);

	/**
	 * @brief Returns the inclusive geotile range [OutLower, OutUpper) for the tile containing the point.
	 *
	 * @param Lat       Latitude in decimal degrees.
	 * @param Lng       Longitude in decimal degrees.
	 * @param TileZoom  Zoom level at which to compute the tile.
	 * @param OutLower  Inclusive lower bound.
	 * @param OutUpper  Exclusive upper bound.
	 * @param MaxZoom   Zoom level at which geotiles are stored in Ditto (default 18 —
	 *                  verified against a live "traci" vehicle's attributes.geotile value).
	 */
	static void GetTileBounds(double Lat, double Lng, int32 TileZoom, int64& OutLower, int64& OutUpper, int32 MaxZoom = 18);

	/** Converts a quadkey + zoom back to geotile bounds without needing lat/lng. */
	static void GetTileBoundsFromKey(int64 QuadKey, int32 TileZoom, int64& OutLower, int64& OutUpper, int32 MaxZoom = 18);

	/** Same as GetThingsByGeotile but takes pre-computed geotile bounds. */
	void GetThingsByGeotileBounds(
		int64 Lower,
		int64 Upper,
		TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
		TFunction<void()> OnCompleted);

	/**
	 * @brief Maps a camera altitude above the WGS-84 ellipsoid to a quadtile zoom level.
	 *
	 * @param AltitudeMeters  Camera altitude in metres.
	 * @return Zoom level in [0, MaxTileZoom].
	 */
	static int32 AltitudeToZoomLevel(double AltitudeMeters);

	static constexpr int32 MaxTileZoom = 20;

private:
	FString WsStartMessage;

	/** Host from the DA_DittoSecrets DataAsset, if present — only used to prefill the login form. */
	FString DefaultHost;

	bool bUseHttps = true;

	/**
	 * @brief The single implementation of the backend-contact seam. Constructed in Initialize()
	 *        as a UDittoRestClient; every method above forwards to it. RestClientOwner is the
	 *        GC-visible handle (declared as UObject so this header does not need to know the
	 *        concrete implementation type); RestClient is the interface view of the same object.
	 */
	UPROPERTY()
	TObjectPtr<UObject> RestClientOwner;

	IBackendRestClient* RestClient = nullptr;
};
