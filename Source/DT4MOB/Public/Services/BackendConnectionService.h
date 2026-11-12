// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

/**
 * @file
 * @brief Declares UBackendConnectionService (login, auth header and REST access to the twin
 *        backend) and its auth delegates.
 * @ingroup services
 */

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BackendConnectionService.generated.h"

class IBackendRestClient;
class FJsonObject;

/**
 * @brief Fires when a usable Authorization header becomes available or changes.
 *
 * Broadcast through UBackendConnectionService::OnAuthHeaderReady after every successful OAuth
 * token grant or refresh, and once per Login() call when HTTP Basic auth is configured.
 *
 * @param AuthHeader  Complete header value, `Bearer <access token>` or `Basic <base64>`.
 * @ingroup services
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBackendAuthHeaderReady, const FString&, AuthHeader);

/**
 * @brief Fires from UBackendConnectionService::Logout() after the in-memory credentials and
 *        tokens have been cleared.
 * @ingroup services
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBackendLoggedOut);

/**
 * @brief GameInstance subsystem that owns the connection to the digital twin backend: login,
 *        the current Authorization header, and REST access to things.
 *
 * The protocol lives behind IBackendRestClient. Initialize() creates a UDittoRestClient (Eclipse
 * Ditto `/api/2` REST API, authenticated with a Keycloak OAuth2 password grant or with HTTP
 * Basic) and every request method here forwards to it. This class keeps the lifecycle, the
 * delegates, the connection settings read from the DA_DittoSecrets DataAsset, and the geotile
 * math.
 *
 * Credentials arrive at runtime through Login(): ULoginWidget calls it with what the user typed,
 * and ALoginPlayerController calls it with credentials restored from UCredentialStoreService.
 * Initialize() does not authenticate. It only reads the non-sensitive settings (default host,
 * bUseHttps, bUseOAuth, OAuthClientId, WsStartMessage) from `/Game/Data/DA_DittoSecrets` when
 * that asset exists.
 *
 * In OAuth mode, requests made before a token is available are queued by the REST client and
 * sent once the token arrives. The access token is refreshed 30 s before it expires with the
 * Keycloak refresh token. Tokens are never written to disk; only Host/Username/Password are
 * persisted (encrypted, by UCredentialStoreService), so every launch performs a fresh password
 * grant.
 *
 * @ingroup services
 */
UCLASS()
class DT4MOB_API UBackendConnectionService : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * @brief Reads the connection settings from DA_DittoSecrets (when present) and creates the
	 *        REST client. Does not log in.
	 *
	 * @note A missing asset is not an error: HTTPS and OAuth stay enabled, OAuthClientId and
	 *       WsStartMessage stay empty, and the host must come from the login screen.
	 *
	 * @param Collection  Subsystem collection supplied by the engine.
	 */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** @brief Shuts down the REST client (cancelling its token refresh timer) and releases it. */
	virtual void Deinitialize() override;

	/**
	 * @brief Broadcast whenever a usable Authorization header is available.
	 *
	 * Fires after each successful OAuth token grant or refresh, or synchronously inside Login()
	 * when Basic auth is configured. UWSService binds here to open its socket, and to pick up
	 * the new header for later reconnects.
	 */
	UPROPERTY(BlueprintAssignable)
	FOnBackendAuthHeaderReady OnAuthHeaderReady;

	/** @brief Broadcast by Logout(). UWSService binds here to close its socket. */
	UPROPERTY(BlueprintAssignable)
	FOnBackendLoggedOut OnLoggedOut;

	/**
	 * @brief Authenticates with the given credentials. Entry point for the login screen and for
	 *        the silent login with stored credentials.
	 *
	 * Stores Host/Username/Password in the REST client and builds the base URL from Host and
	 * bUseHttps. With OAuth it sends a password grant to
	 * `<scheme>://<host>/auth/realms/dt4mob/protocol/openid-connect/token`; with Basic auth the
	 * header is available immediately and the server is not contacted, so wrong credentials only
	 * show up as failed requests later. Can be called again to retry with other credentials.
	 *
	 * @note If a token request is already in flight (for example one started by a queued API
	 *       call), the new grant is skipped and OnComplete is never invoked.
	 *
	 * @param InHost      Backend host name without scheme (e.g. "your-ditto-host.example.com").
	 * @param InUsername  Keycloak (OAuth) or Ditto (Basic) username.
	 * @param InPassword  Password for InUsername.
	 * @param OnComplete  Called with true once an auth header is available, or false when the
	 *                    token request fails or no REST client exists. Called synchronously in
	 *                    Basic mode.
	 */
	void Login(const FString& InHost, const FString& InUsername, const FString& InPassword,
		TFunction<void(bool bSuccess)> OnComplete);

	/**
	 * @brief Clears the in-memory credentials, tokens and queued requests, cancels the token
	 *        refresh timer, and broadcasts OnLoggedOut.
	 *
	 * Does not touch stored credentials; callers also call UCredentialStoreService::ClearCredentials().
	 * The host from the last Login() is kept.
	 */
	void Logout();

	/**
	 * @brief Reports whether requests can currently be authenticated.
	 * @return With OAuth, true while an access token is held. With Basic auth, true from Login()
	 *         until Logout(), whether or not the credentials are valid.
	 */
	bool IsAuthenticated() const;

	/**
	 * @brief Returns the Authorization header value to send with backend requests.
	 * @return `Bearer <access token>` with OAuth (empty until a token has been obtained), or
	 *         `Basic <base64 of username:password>` with Basic auth. Empty if no REST client exists.
	 */
	FString GetCurrentAuthHeader() const;

	/**
	 * @brief Returns the backend host in use.
	 * @return Host passed to the most recent Login() (kept after Logout()), or an empty string
	 *         before the first login.
	 */
	FString GetHost() const;

	/**
	 * @brief Returns the host configured on the DA_DittoSecrets asset, used to prefill the login form.
	 * @return The asset's Host value, or an empty string if the asset is missing or leaves it blank.
	 */
	FString GetDefaultHost() const { return DefaultHost; }

	/**
	 * @brief Returns whether REST and WebSocket URLs use TLS (https/wss).
	 * @return The asset's bUseHttps value, or true if the asset is missing.
	 */
	bool IsUseHttps() const { return bUseHttps; }

	/**
	 * @brief Returns the command UWSService sends after connecting when no event filter is set.
	 * @return The asset's WsStartMessage (normally `START-SEND-EVENTS`), or an empty string if
	 *         the asset is missing.
	 */
	FString GetWsStartMessage() const { return WsStartMessage; }

	/**
	 * @brief Fetches every thing matching a search filter, page by page, through Ditto's search API.
	 *
	 * Pages are requested one after another with Ditto's cursor until no cursor is returned.
	 *
	 * @note In OAuth mode the requests wait in the queue until a token is available. If login
	 *       never succeeds, neither callback fires.
	 *
	 * @param OnPageReceived  Called once per page (up to 50 things) with the parsed thing objects.
	 * @param OnCompleted     Called once after the last page, after the first failed request
	 *                        (transport error, non-200 status or unparsable JSON), or immediately
	 *                        if no REST client exists.
	 * @param Filter          Ditto RQL filter, e.g. `like(thingId,"*sinalizacao*")`. Appended to
	 *                        the URL without URL encoding. Empty returns every thing.
	 */
	void GetAllThings(
		TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
		TFunction<void()> OnCompleted,
		const FString& Filter = FString());

	/**
	 * @brief Fetches the things whose `attributes/geotile` falls inside the tile that contains a
	 *        geographic point.
	 *
	 * Computes the bounds with GetTileBounds() at the default storage zoom (18) and forwards to
	 * GetThingsByGeotileBounds().
	 *
	 * @param Lat             Latitude of the camera or viewport centre in decimal degrees.
	 * @param Lng             Longitude of the camera or viewport centre in decimal degrees.
	 * @param TileZoom        Zoom level that sets the tile size (higher = smaller tile).
	 * @param OnPageReceived  Called with each page of matching thing objects.
	 * @param OnCompleted     Called once when the fetch has finished or failed.
	 */
	void GetThingsByGeotile(
		double Lat,
		double Lng,
		int32 TileZoom,
		TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
		TFunction<void()> OnCompleted);

	/**
	 * @brief Fetches a single thing by its full identifier.
	 *
	 * @param ThingId     Full Ditto thing identifier (e.g. "traci:vehicle-42"). URL-encoded by
	 *                    the REST client.
	 * @param OnComplete  Called with the thing JSON, or nullptr on any failure (non-200 status,
	 *                    unparsable JSON, or no REST client).
	 */
	void GetThingById(
		const FString& ThingId,
		TFunction<void(TSharedPtr<FJsonObject>)> OnComplete);

	/**
	 * @brief Creates or replaces a thing with an HTTP PUT to `/api/2/things/<ThingId>`.
	 *
	 * @param ThingId     Full Ditto thing identifier (e.g. "ignition-point:<guid>").
	 * @param Body        Complete thing JSON to send. Must not be null.
	 * @param OnComplete  Called with true for any 2xx response, false otherwise. Ditto's error
	 *                    body is written to the log on failure.
	 */
	void PutThing(
		const FString& ThingId,
		TSharedPtr<FJsonObject> Body,
		TFunction<void(bool bSuccess)> OnComplete);

	/**
	 * @brief Computes the geotile (quadkey integer) of the tile that contains a geographic point.
	 *
	 * Forwards to FGeotileUtils::LatLonToGeotile().
	 *
	 * @param Lat   Latitude in decimal degrees.
	 * @param Lng   Longitude in decimal degrees.
	 * @param Zoom  Zoom level, 0 to 31.
	 * @return      64-bit quadkey integer.
	 */
	static int64 GetQuadkey(double Lat, double Lng, int32 Zoom);

	/**
	 * @brief Converts a geographic point to slippy map tile indices. Forwards to
	 *        FGeotileUtils::LatLonToTileXY().
	 *
	 * @param Lat   Latitude in decimal degrees.
	 * @param Lng   Longitude in decimal degrees.
	 * @param Zoom  Zoom level.
	 * @param OutX  Receives the tile column, 0 to 2^Zoom - 1.
	 * @param OutY  Receives the tile row, 0 to 2^Zoom - 1 (0 = north).
	 */
	static void GetTileXY(double Lat, double Lng, int32 Zoom, int64& OutX, int64& OutY);

	/**
	 * @brief Converts tile indices to a quadkey integer. Forwards to FGeotileUtils::TileXYToQuadkey().
	 *
	 * @param X     Tile column at Zoom.
	 * @param Y     Tile row at Zoom.
	 * @param Zoom  Zoom level, 0 to 31.
	 * @return      64-bit quadkey integer.
	 */
	static int64 GetQuadkeyFromXY(int64 X, int64 Y, int32 Zoom);

	/**
	 * @brief Returns the geotile range [OutLower, OutUpper) covered by the tile that contains a
	 *        geographic point. Forwards to FGeotileUtils::GeotileBounds().
	 *
	 * @param Lat       Latitude in decimal degrees.
	 * @param Lng       Longitude in decimal degrees.
	 * @param TileZoom  Zoom level at which to compute the tile.
	 * @param OutLower  Receives the inclusive lower bound.
	 * @param OutUpper  Receives the exclusive upper bound.
	 * @param MaxZoom   Zoom level at which the geotile values were stored (default 18, the
	 *                  precision of TRACI vehicles, checked against a live "traci" vehicle).
	 */
	static void GetTileBounds(double Lat, double Lng, int32 TileZoom, int64& OutLower, int64& OutUpper, int32 MaxZoom = 18);

	/**
	 * @brief Returns the geotile range [OutLower, OutUpper) covered by a tile given by its quadkey.
	 *
	 * Same result as GetTileBounds() for a point inside that tile, without needing the point.
	 * When TileZoom is finer than MaxZoom, the range collapses to the single MaxZoom tile that
	 * contains the requested tile.
	 *
	 * @param QuadKey   Quadkey of the tile at TileZoom (from GetQuadkey() or GetQuadkeyFromXY()).
	 * @param TileZoom  Zoom level of QuadKey.
	 * @param OutLower  Receives the inclusive lower bound.
	 * @param OutUpper  Receives the exclusive upper bound.
	 * @param MaxZoom   Zoom level at which the geotile values were stored (default 18).
	 */
	static void GetTileBoundsFromKey(int64 QuadKey, int32 TileZoom, int64& OutLower, int64& OutUpper, int32 MaxZoom = 18);

	/**
	 * @brief Fetches the things whose `attributes/geotile` lies in [Lower, Upper), page by page.
	 *
	 * The search filter also drops things whose `attributes/expiry_ts` is more than 30 s in the
	 * past (things without expiry_ts are kept), so finished simulation runs left in Ditto are not
	 * spawned again. Pages hold up to 200 things.
	 *
	 * @param Lower           Inclusive lower geotile bound.
	 * @param Upper           Exclusive upper geotile bound.
	 * @param OnPageReceived  Called with each page of matching thing objects.
	 * @param OnCompleted     Called once after the last page, after the first failed request, or
	 *                        immediately if no REST client exists.
	 */
	void GetThingsByGeotileBounds(
		int64 Lower,
		int64 Upper,
		TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
		TFunction<void()> OnCompleted);

	/**
	 * @brief Maps a camera altitude above the WGS-84 ellipsoid to a tile zoom level.
	 *
	 * Uses round(log2(10019000 / AltitudeMeters)), so zoom 0 is at about 10,000 km (a quarter of
	 * the equator) and each halving of the altitude adds one level.
	 *
	 * @param AltitudeMeters  Camera altitude in metres.
	 * @return                Zoom level clamped to 0 to MaxTileZoom; MaxTileZoom for altitudes
	 *                        of 1 m or less.
	 */
	static int32 AltitudeToZoomLevel(double AltitudeMeters);

	/** @brief Highest zoom level AltitudeToZoomLevel() returns. */
	static constexpr int32 MaxTileZoom = 20;

private:
	/** @brief WebSocket start command from DA_DittoSecrets; empty when the asset is missing. */
	FString WsStartMessage;

	/** @brief Host from DA_DittoSecrets, only used to prefill the login form. */
	FString DefaultHost;

	/** @brief From DA_DittoSecrets (true when the asset is missing). Also passed to the REST client. */
	bool bUseHttps = true;

	/**
	 * @brief GC-visible owner of the REST client, a UDittoRestClient created in Initialize().
	 *
	 * Declared as UObject so this header does not depend on the concrete client type.
	 * RestClient is the interface view of the same object.
	 */
	UPROPERTY()
	TObjectPtr<UObject> RestClientOwner;

	/** @brief Interface pointer to RestClientOwner. Every request method forwards here; null after Deinitialize(). */
	IBackendRestClient* RestClient = nullptr;
};
