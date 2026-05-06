// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DT4MOBGamemode.generated.h"

/**
 * @brief Default game mode for the DT4MOB project.
 *
 * On BeginPlay it bootstraps the runtime by:
 *  1. Starting the WebSocket connection via UEntityUpdateDaemon so live Ditto events
 *     are received and routed to registered actors.
 *  2. Fetching the initial snapshot of all Ditto things via UDittoService and spawning
 *     an ATempUIActor for each recognised thing through UDT4MOBEntityFactory.
 *
 * The Daemon handles socket teardown automatically on Deinitialize(), so EndPlay
 * requires no explicit cleanup.
 */
UCLASS()
class DT4MOB_API ADT4MOBGamemode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** @brief Enables ticking (retained for future per-frame logic). */
	ADT4MOBGamemode();

	/**
	 * @brief Called every frame (currently a passthrough to Super).
	 * @param DeltaSeconds Time elapsed since the last frame.
	 */
	virtual void Tick(float DeltaSeconds) override;

protected:
	/**
	 * @brief Starts the WebSocket connection and fetches the initial entity snapshot.
	 *
	 * Spawning is dispatched back to the GameThread via AsyncTask so it is safe to
	 * call from any thread context returned by the HTTP response handler.
	 */
	virtual void BeginPlay() override;

	/**
	 * @brief Called when play ends; no explicit cleanup needed as the Daemon handles socket teardown.
	 * @param EndPlayReason The reason play is ending.
	 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/**
	 * @brief Legacy helper used to spawn actors for a batch of raw FJsonValue things.
	 *
	 * Kept for compatibility with older code paths. New code should use the lambda-based
	 * GetAllThings() callback that receives TSharedPtr<FJsonObject> directly.
	 *
	 * @param Things Array of JSON values representing Ditto things.
	 */
	void OnCompletedGetAllThings(const TArray<TSharedPtr<FJsonValue>> &Things);

	/**
	 * @brief Called every tick to check whether the camera has moved into a new tile
	 *        and schedule a debounced geotile refresh when it has.
	 * @param DeltaSeconds Time since last frame.
	 */
	void CheckAndRefreshTiles(float DeltaSeconds);

	/**
	 * @brief Destroys all existing entity actors and fetches the new tile's things from Ditto.
	 * @param Lat      Latitude of the tile centre in decimal degrees.
	 * @param Lng      Longitude of the tile centre in decimal degrees.
	 * @param TileZoom Quadtile zoom level to query.
	 */
	void DoTileRefresh(double Lat, double Lng, int32 TileZoom);

	// ---- Tile state ----

	/** @brief Zoom level of the last executed geotile query (-1 = not yet queried). */
	int32 LastTileZoom = -1;

	/** @brief Lower geotile bound of the last executed query. */
	int64 LastTileLower = -1;

	/** @brief Upper geotile bound of the last executed query. */
	int64 LastTileUpper = -1;

	/** @brief Whether a tile refresh is waiting for the debounce timer. */
	bool bPendingTileRefresh = false;

	/** @brief Countdown in seconds until the pending tile refresh fires. */
	float TileRefreshTimer = 0.f;

	/** @brief Lat/Lng/Zoom captured when the current debounce started. */
	double PendingLat = 0.0;
	double PendingLng = 0.0;
	int32 PendingZoom = 0;

	/** @brief Tile bounds captured when the current debounce started. */
	int64 PendingTileLower = -1;
	int64 PendingTileUpper = -1;

	/** @brief Seconds of camera stability required before firing a tile refresh. */
	static constexpr float TileRefreshDelay = 0.75f;

	/** @brief Minimum zoom level before tile filtering activates (below this, load everything). */
	static constexpr int32 MinZoomForTileFiltering = 7;
};
