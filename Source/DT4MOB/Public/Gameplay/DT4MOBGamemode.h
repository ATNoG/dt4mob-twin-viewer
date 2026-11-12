// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares ADT4MOBGamemode, the main-scene game mode that drives tile-based entity streaming.
 * @ingroup gameplay
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DT4MOBGamemode.generated.h"

/**
 * @brief Game mode for the main world scene. Decides which Ditto things are loaded into the world.
 *
 * Responsibilities:
 *  - Tile streaming: every TileCheckInterval seconds it converts the pawn position and spring-arm
 *    length into a camera altitude, maps that to a quadtile zoom level, and (after a debounce)
 *    loads the 3x3 tile neighbourhood around the camera through UBackendConnectionService and
 *    UDT4MOBEntityFactory. The live WebSocket subscription filter is narrowed to the same tiles.
 *  - On-demand spawning: listens to UEntityUpdateDaemon::OnUnhandledThingMessage and spawns actors
 *    for types whose extension opts in via UEntityTypeExtension::AllowsOnDemandSpawnFromWS().
 *  - Orphan cleanup: periodically calls UDT4MOBEntityFactory::SweepOrphanedActors().
 *
 * This class does not open the WebSocket connection, and socket teardown is left to the
 * subsystems, so EndPlay has nothing to release.
 *
 * @ingroup gameplay
 */
UCLASS()
class DT4MOB_API ADT4MOBGamemode : public AGameModeBase
{
	GENERATED_BODY()

public:
	/** @brief Enables actor ticking, which drives the tile check and the orphan sweep. */
	ADT4MOBGamemode();

	/**
	 * @brief Runs the throttled tile check (when bUseTileStreaming is true) and the periodic orphan sweep.
	 * @param DeltaSeconds Time elapsed since the last frame, in seconds.
	 */
	virtual void Tick(float DeltaSeconds) override;

protected:
	/**
	 * @brief Subscribes to unhandled WebSocket messages and, if tile streaming is disabled, runs the one-shot full fetches.
	 *
	 * The full fetch only happens when bUseTileStreaming is false: every registered type whose
	 * extension returns true from RequiresFullFetch() is fetched once with its GetFullFetchFilter()
	 * RQL filter. Pages arrive on the HTTP thread and are forwarded to the game thread with
	 * AsyncTask before actors are spawned. With tile streaming enabled (the default), BeginPlay
	 * only binds HandleUnhandledThingMessage and the first load happens from Tick.
	 */
	virtual void BeginPlay() override;

	/**
	 * @brief Forwards to the base class; no gamemode-owned resources need releasing.
	 * @param EndPlayReason Why play is ending.
	 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/**
	 * @brief Handles a WebSocket event for a thingId that has no actor in the world yet.
	 *
	 * Ignored unless the factory recognises the thingId and its type extension allows on-demand
	 * spawning from WebSocket events. Two paths follow:
	 *  - Full-thing events (Path == "/") are spawned straight from the payload (after injecting the
	 *    thingId and skipping things whose `attributes.expiry_ts` is more than 30 s in the past),
	 *    then enriched with a best-effort GetThingById() so static attributes arrive too.
	 *  - Anything else triggers GetThingById(); once it returns, the actor is spawned on the game
	 *    thread and the triggering update is replayed through UEntityUpdateDaemon::InjectUpdate().
	 *
	 * PendingOnDemandSpawns prevents more than one REST fetch per thingId on the second path.
	 *
	 * @param ThingId    Ditto thingId taken from the event topic.
	 * @param Path       Ditto JSON pointer the event applies to ("/" for a full thing).
	 * @param ValueJson  Serialized JSON value carried by the event.
	 */
	UFUNCTION()
	void HandleUnhandledThingMessage(const FString &ThingId, const FString &Path, const FString &ValueJson);

	/**
	 * @brief Spawns an actor for each thing in a batch of raw JSON values.
	 *
	 * Each value is converted to a JSON object and spawned through UDT4MOBEntityFactory on the
	 * game thread. Nothing in the current code calls this; the tile and full-fetch paths pass
	 * lambdas that receive TSharedPtr<FJsonObject> pages directly.
	 *
	 * @param Things Array of JSON values representing Ditto things.
	 */
	void OnCompletedGetAllThings(const TArray<TSharedPtr<FJsonValue>> &Things);

	/**
	 * @brief Recomputes the camera zoom level and tile set, and schedules or fires a debounced tile refresh.
	 *
	 * Camera altitude is the pawn's height above the WGS-84 ellipsoid plus the target spring-arm
	 * length (converted from cm to m). A new zoom level only takes effect after it has held for
	 * ZoomChangeHoldSeconds. Below MinZoomForTileFiltering no tiles are fetched; the WebSocket
	 * filter is switched to ZoomedOutEventFilter and actors already in the world are left as they
	 * are. Otherwise, once the same 3x3 tile set has been requested for TileRefreshDelay seconds,
	 * DoTileRefresh() runs.
	 *
	 * @param DeltaSeconds Time since the previous check, in seconds (Tick passes TileCheckInterval).
	 */
	void CheckAndRefreshTiles(float DeltaSeconds);

	/**
	 * @brief Applies a new visible tile set: updates the WebSocket filter, unloads stale tiles and fetches new ones.
	 *
	 * The WebSocket filter becomes an RQL `or()` of geotile ranges covering every tile at every
	 * precision in GeotileStorageZooms. If the zoom level changed, all entity actors are destroyed
	 * and every tile is fetched again; otherwise only tiles leaving the set are unloaded and only
	 * tiles entering it are fetched. Fetched pages are spawned on the game thread through
	 * UDT4MOBEntityFactory::SpawnTempUIActorForTile().
	 *
	 * @param NewTileKeys  Quadkeys of the tiles that should be loaded.
	 * @param Zoom         Zoom level the quadkeys were computed at.
	 */
	void DoTileRefresh(const TSet<int64>& NewTileKeys, int32 Zoom);

	/**
	 * @brief Returns the quadkeys of the 3x3 tile grid centred on the given position.
	 *
	 * Tile X/Y are clamped to the valid grid, so at the edges of the map some neighbours collapse
	 * onto the same tile and fewer than nine keys are returned.
	 *
	 * @param Lat   Latitude in decimal degrees.
	 * @param Lng   Longitude in decimal degrees.
	 * @param Zoom  Quadtile zoom level.
	 * @return Set of quadkeys (one to nine entries).
	 */
	TSet<int64> GetNeighborTileKeys(double Lat, double Lng, int32 Zoom) const;

	// ---- Tile state ----

	/** @brief Quadkeys of the tiles currently loaded in the world. */
	TSet<int64> LoadedTileKeys;

	/** @brief Zoom level at which LoadedTileKeys were fetched, or -1 before the first refresh. */
	int32 LoadedZoom = -1;

	/** @brief True while a tile refresh is waiting for the debounce timer to run out. */
	bool bPendingTileRefresh = false;

	/** @brief Seconds left before the pending tile refresh fires. */
	float TileRefreshTimer = 0.f;

	/** @brief Tile set captured when the current debounce started. */
	TSet<int64> PendingTileKeys;

	/** @brief Zoom level captured when the current debounce started. */
	int32 PendingZoom = 0;

	/**
	 * @brief Raw zoom level that differs from LoadedZoom and is waiting to be committed, or -1 if none.
	 *
	 * Zoom-change hysteresis: camera-altitude jitter around a zoom boundary would otherwise flip
	 * the level back and forth, and every flip runs DestroyAllActors() and wipes the whole fleet.
	 */
	int32 CandidateZoom = -1;

	/** @brief Seconds CandidateZoom still has to hold before it replaces LoadedZoom. */
	float ZoomHoldTimer = 0.f;

	/** @brief How long, in seconds, a new zoom level must persist before it is committed. */
	static constexpr float ZoomChangeHoldSeconds = 2.0f;

	/** @brief Seconds the requested tile set must stay unchanged before a tile refresh fires. */
	static constexpr float TileRefreshDelay = 0.75f;

	/** @brief Interval, in seconds, between tile checks. */
	static constexpr float TileCheckInterval = 0.5f;

	/** @brief Seconds left until the next tile check. */
	float TileCheckTimer = 0.f;

	/**
	 * @brief Lowest zoom level at which tiles are fetched.
	 *
	 * Below this level CheckAndRefreshTiles() stops fetching tiles and only switches the
	 * WebSocket filter to ZoomedOutEventFilter.
	 */
	static constexpr int32 MinZoomForTileFiltering = 7;

	/**
	 * @brief RQL filter applied to the WebSocket subscription when zoomed out below MinZoomForTileFiltering.
	 *
	 * Individual simulated vehicles are sub-pixel at that range and their high-churn update
	 * stream is the largest share of WebSocket traffic, so the whole `traci:` namespace is
	 * excluded instead of subscribing unfiltered. An empty string would mean no filter at all.
	 */
	static inline const FString ZoomedOutEventFilter = TEXT("not(like(thingId,\"traci:*\"))");

	/**
	 * @brief Master switch for tile streaming.
	 *
	 * When false, Tick skips the tile check and BeginPlay instead does a single fetch, not filtered
	 * by geotile, for every type whose extension requires a full fetch (currently "sinalizacao"
	 * road signs).
	 */
	static constexpr bool bUseTileStreaming = true;

	/** @brief Interval, in seconds, between calls to UDT4MOBEntityFactory::SweepOrphanedActors(). */
	static constexpr float OrphanSweepInterval = 2.0f;

	/** @brief Seconds left until the next orphan sweep. */
	float OrphanSweepTimer = 0.f;

	/**
	 * @brief Zoom precisions at which the different Ditto data sources encode `attributes/geotile`.
	 *
	 * TRACI-simulated vehicles use 18 (verified against a live vehicle payload) and static
	 * infrastructure imports use 31. Every tile fetch and WebSocket filter covers each of these
	 * so a tile's entities are found whichever source encoded them.
	 */
	static inline const TArray<int32> GeotileStorageZooms = { 18, 31 };

	/**
	 * @brief ThingIds with an on-demand GetThingById() in flight from HandleUnhandledThingMessage().
	 *
	 * Stops a burst of WebSocket events for the same not-yet-spawned thing from firing one
	 * redundant REST request per message while the first one is still pending.
	 */
	TSet<FString> PendingOnDemandSpawns;
};
