// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "EntityStructs/MeteorologyStruct.h"
#include "Services/EntityUpdateDaemon.h"
#include "CesiumSampleHeightMostDetailedAsyncAction.h"
#include "Cesium3DTileset.h"
#include "CesiumGlobeAnchorComponent.h"
#include "TempUIActor.generated.h"

class ACesiumCartographicPolygon;

/**
 * @brief Actor that represents a live Ditto twin entity in the world.
 *
 * Each instance is associated with exactly one Ditto thingId. On spawn it is
 * initialised with a UScriptStruct type and a JSON payload via Initialize().
 * The actor registers with UEntityUpdateDaemon so that live WebSocket events
 * are forwarded to HandleEntityUpdate(), which patches RawJson and re-serialises
 * the struct so downstream UI always reflects the latest state.
 *
 * Interaction (hover / selection) is handled through USelectionManager delegates
 * and reflected via custom depth stencil values on all primitive components.
 *
 * @note The actor does not tick by default. All updates arrive through the Daemon.
 */
UCLASS()
class DT4MOB_API ATempUIActor : public AActor
{
	GENERATED_BODY()

protected:
	/** @brief Registers with SelectionManager and EntityUpdateDaemon. */
	virtual void BeginPlay() override;

	/** @brief Unregisters from SelectionManager and EntityUpdateDaemon. @param EndPlayReason Reason for ending play. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/** @brief Sets up default components (SceneRoot, InteractionBox, StaticMesh). */
	ATempUIActor();

	// ---- Components ----

	/** @brief Keeps the actor pinned to its geographic coordinates through Cesium origin rebasing. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UCesiumGlobeAnchorComponent *GlobeAnchor;

	/** @brief Root scene component used as the actor's transform anchor. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USceneComponent *SceneRoot;

	/** @brief Box collision component used to detect cursor hover via a custom trace channel. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UBoxComponent *InteractionBox;

	/** @brief Visual mesh shown in the world; defaults to the engine Cube. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent *StaticMeshComponent;


	// ---- Data ----

	/** @brief Reflected struct type used to deserialise the Ditto thing JSON at runtime. */
	UPROPERTY()
	UScriptStruct *DataStructType;

	/** @brief Live struct instance populated from RawJson via FJsonObjectConverter. */
	TSharedPtr<FStructOnScope> StructInstance;

	/** @brief Raw JSON representation of the Ditto thing, kept in sync with live updates. */
	TSharedPtr<FJsonObject> RawJson;

	/**
	 * @brief Initialises the actor with a struct type and a parsed JSON object.
	 *
	 * Extracts the thingId, places the actor at the geographic coordinates found
	 * inside the struct, and registers with the EntityUpdateDaemon if BeginPlay
	 * has already run.
	 *
	 * @param InType      UScriptStruct that describes the shape of the Ditto thing.
	 * @param JsonObject  Parsed JSON payload for the thing.
	 */
	void Initialize(UScriptStruct *InType, TSharedPtr<FJsonObject> JsonObject);

	/**
	 * @brief Serialises RawJson back to a compact JSON string.
	 * @return JSON string representation of the current entity state, or empty if invalid.
	 */
	FString GetJsonString() const;

	/**
	 * @brief Sets an arbitrary info text string on the actor.
	 * @param NewInfoText Text to store.
	 */
	void SetInfoText(const FString &NewInfoText) { InfoText = NewInfoText; }

	// ---- Daemon delegate ----

	/**
	 * @brief Delegate used by the EntityUpdateDaemon to deliver per-thingId events.
	 *
	 * The Daemon holds a raw pointer to this delegate. Register/Unregister are
	 * called in BeginPlay/EndPlay to keep the lifetime safe.
	 */
	UPROPERTY()
	FOnEntityUpdated OnEntityUpdated;

	// ---- Live data changed ----

	/**
	 * @brief Broadcast on the GameThread whenever RawJson is patched.
	 *
	 * Any open UI widgets bound to this actor should subscribe to refresh their
	 * displayed data without polling.
	 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEntityDataChanged);

	/** @brief Multicast delegate broadcast after every successful data patch. */
	UPROPERTY(BlueprintAssignable)
	FOnEntityDataChanged OnEntityDataChanged;

	/** @brief Returns the Ditto thingId for this actor. */
	UFUNCTION(BlueprintCallable)
	const FString &GetThingId() const { return ThingId; }

	/**
	 * @brief Reads a string value from RawJson by dot-separated path (e.g. "attributes.matricula").
	 * Returns an empty string if the path doesn't exist or the value isn't a string.
	 */
	UFUNCTION(BlueprintCallable, Category = "Entity")
	FString GetRawJsonField(const FString &DotPath) const;

private:
	/** @brief The Ditto thingId string (e.g. "tolls:toll-1"), extracted during Initialize(). */
	FString ThingId;

	/** @brief True when the cursor is currently hovering over this actor. */
	bool IsHovered = false;

	/** @brief True when this actor is the currently selected entity. */
	bool IsSelected = false;

	/** @brief Arbitrary info text set via SetInfoText(). */
	FString InfoText;

	/** @brief Enables or disables custom-depth rendering on all primitive components to show hover/selection feedback. */
	void UpdateHighlight();

	// ---- Selection callbacks ----

	/**
	 * @brief Called by USelectionManager when the hovered actor changes.
	 * @param NewHoveredActor The newly hovered actor, or nullptr if none.
	 */
	UFUNCTION()
	void HandleHoverChanged(AActor *NewHoveredActor);

	/**
	 * @brief Called by USelectionManager when the selected actor changes.
	 * @param NewSelectedActor The newly selected actor, or nullptr if none.
	 */
	UFUNCTION()
	void HandleSelectedChanged(AActor *NewSelectedActor);

	// ---- Daemon callback ----
	/**
	 * Invoked by EntityUpdateDaemon for every Ditto event matching our ThingId.
	 *
	 * @param Path      Ditto path, e.g. "/features/lidar-outsight/properties/1"
	 * @param ValueJson The "value" field from the envelope as a JSON string.
	 */
	UFUNCTION()
	void HandleEntityUpdate(const FString &Path, const FString &ValueJson);

	// ---- Update routing ----

	/** @brief Full thing or attribute replace — merge into RawJson + re-deserialise struct. */
	void ApplyFullOrAttributePatch(TSharedPtr<FJsonObject> ValueObject, const FString &Path);

	/** @brief Primitive leaf value (string, number, bool) — navigate RawJson by path and set the field. */
	void ApplyLeafPatch(const FString &Path, TSharedPtr<FJsonValue> Value);

	/**
	 * @brief Feature property update.
	 *
	 * Path format: "/features/<featureName>/properties/<entryId>"
	 * Merges the new property entry into RawJson["features"][featureName]["properties"][entryId].
	 *
	 * @param FeatureName  Name of the Ditto feature (e.g. "lidar-outsight").
	 * @param EntryId      Key within the feature's properties map (e.g. "1").
	 * @param ValueObject  Parsed JSON object to merge.
	 */
	void ApplyFeaturePatch(const FString &FeatureName,
						   const FString &EntryId,
						   TSharedPtr<FJsonObject> ValueObject);

	/**
	 * @brief Handles "/features/<featureName>/properties" paths (no entryId).
	 *
	 * Replaces RawJson["features"][featureName]["properties"] with ValueObject,
	 * re-deserialises the struct, and repositions the actor from absoluteCoordinates
	 * if present at the value root (camera-ORT / State feature format).
	 */
	void ApplyFeaturePropertiesPatch(const FString &FeatureName, TSharedPtr<FJsonObject> ValueObject);

	/** @brief Re-reads coordinates from StructInstance and moves the actor in the world. */
	void SetLocation();

	/** @brief Respawns the terrain exclusion polygon using fire perimeter data. No-op for non-fire entities. */
	void RefreshFireExclusion();

	/**
	 * @brief Reads attributes.polygon from RawJson and, if it is a new URL, asks
	 * UGlbModelService to load (or serve from cache) the mesh and apply it.
	 */
	void TryLoadGlbModel();

	/** @brief Callback from UGlbModelService — applies the loaded mesh to StaticMeshComponent. */
	UFUNCTION()
	void OnPolygonMeshLoaded(UStaticMesh *Mesh);

	/** @brief Last polygon URL successfully requested, used to skip redundant reloads. */
	FString LoadedPolygonUrl;

	/** @brief CartographicPolygon that tells Cesium to skip terrain tiles under this actor's GLB model. Null when no GLB is loaded. */
	UPROPERTY()
	ACesiumCartographicPolygon* TerrainExclusionPolygon = nullptr;

	/** @brief Spawns a rectangular CartographicPolygon around LastLatitude/LastLongitude and registers it with the terrain's PolygonRasterOverlay. */
	void SpawnTerrainExclusionPolygon();

	/** @brief Removes this actor's polygon from the terrain overlay and destroys the actor. */
	void RemoveTerrainExclusionPolygon();

	// ---- Generic visualization helpers ----

	/**
	 * @brief Reads attributes.expiry_ts from RawJson and schedules actor self-destruction.
	 * Works for any entity type that carries a string ISO-8601 expiry timestamp.
	 */
	void TryApplyExpiry();

	/** @brief Timer callback that destroys this actor on expiry. */
	UFUNCTION()
	void OnExpired();

	/**
	 * @brief Scales StaticMeshComponent and InteractionBox to match vehicle bounding-box dimensions.
	 * @param LengthM Forward extent in metres (maps to X scale).
	 * @param WidthM  Lateral extent in metres (maps to Y scale).
	 * @param HeightM Vertical extent in metres (maps to Z scale).
	 */
	void ApplyScale(double LengthM, double WidthM, double HeightM);

	/** @brief One-shot timer that fires when expiry_ts is reached. */
	FTimerHandle ExpiryTimer;

	/**
	 * @brief Launches a UCesiumSampleHeightMostDetailedAsyncAction to snap the actor
	 * to the real terrain surface. Unlike raycasting, this API actively requests tile
	 * data for the position even if tiles are not currently loaded/visible.
	 */
	void SnapToGround();

	/**
	 * @brief Callback invoked when Cesium finishes sampling terrain height.
	 * Converts the sampled geodetic height back to UE world space and moves the actor.
	 */
	UFUNCTION()
	void OnGroundHeightSampled(const TArray<FCesiumSampleHeightResult> &Results, const TArray<FString> &Warnings);

	/** @brief Last known latitude, stored so SnapToGround can issue the height query. */
	double LastLatitude = 0.0;

	/** @brief Last known longitude, stored so SnapToGround can issue the height query. */
	double LastLongitude = 0.0;

	/** @brief Altitude in metres from the entity data, when explicitly provided. */
	double LastExplicitAltitude = 0.0;

	/** @brief True when the entity data contains a valid non-zero altitude — suppresses snap-to-ground. */
	bool bHasExplicitAltitude = false;

	FTimerHandle VisibilityCheckTimer;
	void CheckVisibility();

	bool bSnappedToGround = false;
	bool bSnapInProgress = false;
	double LastSnappedAltitudeMeters = 0.0;
	double LastSnappedLatitude = 0.0;
	double LastSnappedLongitude = 0.0;

	void TriggerSnapIfNeeded();

	// Smooth position interpolation
	double VisualLatitude = 0.0;
	double VisualLongitude = 0.0;
	double InterpolationSpeedKmh = 0.0;
	bool bHasInterpolationTarget = false;
	bool bReceivedFirstLiveUpdate = false;
	double LastTargetSetTime = 0.0;
	double EstimatedUpdateInterval = 1.0;

	void SetMovementTarget(double Lat, double Lon, double SpeedKmh, bool bTeleport = false);

	virtual void Tick(float DeltaTime) override;

	/**
	 * @brief Reads a string-valued property from the live StructInstance by name.
	 * @param PropertyName Case-insensitive property name to look up.
	 * @return The string value, or an empty FString if not found.
	 */
	FString GetStringProperty(const FString &PropertyName);
};
