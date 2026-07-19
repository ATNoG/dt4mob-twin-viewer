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
	 * @brief Called by EntityUpdateDaemon when a WS event arrives for an unknown thingId.
	 *
	 * If the factory can handle the thingId and no fetch is already in flight for it,
	 * fetches the full thing from Ditto, spawns an actor, then replays the pending update.
	 */
	UFUNCTION()
	void HandleUnhandledThingMessage(const FString &ThingId, const FString &Path, const FString &ValueJson);

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

	/** @brief Destroys/fetches only the tiles that changed since the last refresh. */
	void DoTileRefresh(const TSet<int64>& NewTileKeys, int32 Zoom);

	/** @brief Returns the quadkeys for the 3×3 tile grid centred on the given position. */
	TSet<int64> GetNeighborTileKeys(double Lat, double Lng, int32 Zoom) const;

	// ---- Tile state ----

	/** @brief Quadkeys currently loaded in the world. */
	TSet<int64> LoadedTileKeys;

	/** @brief Zoom level at which LoadedTileKeys were fetched (-1 = none). */
	int32 LoadedZoom = -1;

	/** @brief Whether a tile refresh is waiting for the debounce timer. */
	bool bPendingTileRefresh = true;

	/** @brief Countdown in seconds until the pending tile refresh fires. */
	float TileRefreshTimer = 0.f;

	/** @brief Tile set and zoom captured when the current debounce started. */
	TSet<int64> PendingTileKeys;
	int32 PendingZoom = 0;

	/** @brief Seconds of camera stability required before firing a tile refresh. */
	static constexpr float TileRefreshDelay = 0.75f;

	/** @brief How often (in seconds) the tile check runs. */
	static constexpr float TileCheckInterval = 0.5f;

	/** @brief Countdown until the next tile check. */
	float TileCheckTimer = 0.f;

	/** @brief Minimum zoom level before tile filtering activates (below this, load everything). */
	static constexpr int32 MinZoomForTileFiltering = 7;

	/** @brief When false, tile-based streaming is skipped entirely — BeginPlay does a single
	 *         unfiltered-by-geotile fetch instead (currently scoped to "sinalizacao" signs). */
	static constexpr bool bUseTileStreaming = false;

	/** @brief How often (in seconds) orphaned (protected-but-tileless) actors are re-evaluated for cleanup. */
	static constexpr float OrphanSweepInterval = 2.0f;

	/** @brief Countdown until the next orphan sweep. */
	float OrphanSweepTimer = 0.f;
};
