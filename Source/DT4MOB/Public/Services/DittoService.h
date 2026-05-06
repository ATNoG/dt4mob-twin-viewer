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

	/**
	 * @brief Fetches Ditto things whose geotile attribute falls within the tile that contains
	 *        the given geographic point at the specified zoom level.
	 *
	 * Builds the RQL filter:
	 *   and(ge(attributes/geotile,<lower>),le(attributes/geotile,<upper>))
	 * where the bounds are derived from the quadtile that contains (Lat, Lng) at TileZoom,
	 * expressed in the zoom-31 quadkey space used by Ditto.
	 *
	 * Pagination is handled internally; OnPageReceived fires once per page and OnCompleted
	 * fires when all pages have been fetched (or on error).
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
		TFunction<void(const TArray<TSharedPtr<FJsonObject>> &)> OnPageReceived,
		TFunction<void()> OnCompleted);

	/**
	 * @brief Computes the OSM quad-tile integer key for a geographic point at the given zoom level.
	 * Source: https://wiki.openstreetmap.org/wiki/QuadTiles
	 *
	 * @param Lat   Latitude in decimal degrees.
	 * @param Lng   Longitude in decimal degrees.
	 * @param Zoom  Zoom level (0–31).
	 * @return 64-bit quadkey integer.
	 */
	static int64 GetQuadkey(double Lat, double Lng, int32 Zoom);

	/**
	 * @brief Returns the inclusive geotile range [OutLower, OutUpper) for the tile containing the point.
	 *
	 * Bounds are in the zoom-31 quadkey space used by Ditto's attributes/geotile field.
	 *
	 * @param Lat       Latitude in decimal degrees.
	 * @param Lng       Longitude in decimal degrees.
	 * @param TileZoom  Zoom level at which to compute the tile.
	 * @param OutLower  Inclusive lower bound.
	 * @param OutUpper  Exclusive upper bound.
	 * @param MaxZoom   Zoom level at which geotiles are stored in Ditto (default 31).
	 */
	static void GetTileBounds(double Lat, double Lng, int32 TileZoom, int64 &OutLower, int64 &OutUpper, int32 MaxZoom = 31);

	/**
	 * @brief Maps a camera altitude above the WGS-84 ellipsoid to a quadtile zoom level.
	 *
	 * Uses zoom = clamp(round(log2(10,019,000 / altitude_m)), 0, MaxTileZoom).
	 *
	 * @param AltitudeMeters  Camera altitude in metres.
	 * @return Zoom level in [0, MaxTileZoom].
	 */
	static int32 AltitudeToZoomLevel(double AltitudeMeters);

	/** @brief Maximum tile zoom used for geotile queries (caps resolution for Ditto performance). */
	static constexpr int32 MaxTileZoom = 20;

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
