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
};
