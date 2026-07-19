// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "EntityStructs/MeteorologyStruct.h"
#include "Services/EntityUpdateDaemon.h"
#include "Services/GlbModelService.h"
#include "CesiumSampleHeightMostDetailedAsyncAction.h"
#include "Cesium3DTileset.h"
#include "CesiumGlobeAnchorComponent.h"
#include "TempUIActor.generated.h"

class ACesiumCartographicPolygon;
class UMaterialInterface;
class UEntityBehaviorComponent;

/** @brief Cached original material slots for a mesh layer, restored when transparency is toggled off. */
USTRUCT()
struct FMeshLayerMaterialSet
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> Materials;
};

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

	/** @brief Visual mesh shown in the world; defaults to the engine Cube. Hidden when a GLB layer is loaded. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent *StaticMeshComponent;

	/** @brief Optional per-type behavior component, attached generically in Initialize() based on
	 *  UEntityTypeExtension::GetBehaviorComponentClass(). Null for types with no custom behavior. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UEntityBehaviorComponent *BehaviorComponent = nullptr;

	// ---- Mesh layers ----

	/** @brief Named mesh components added via AddOrReplaceMeshLayer (e.g. "Polygon" for fire GLBs). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TMap<FString, UStaticMeshComponent*> MeshLayers;

	/** @brief Broadcast whenever a layer is added, removed, or its visibility changes. */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMeshLayersChanged);

	UPROPERTY(BlueprintAssignable)
	FOnMeshLayersChanged OnMeshLayersChanged;

	/** @brief Returns the names of all current mesh layers. */
	UFUNCTION(BlueprintCallable, Category = "MeshLayers")
	TArray<FString> GetMeshLayerNames() const;

	/** @brief Sets the visibility of a named mesh layer. No-op if the layer doesn't exist. */
	UFUNCTION(BlueprintCallable, Category = "MeshLayers")
	void SetMeshLayerVisible(const FString& LayerName, bool bVisible);

	/** @brief Returns the visibility of a named mesh layer. Returns false if the layer doesn't exist. */
	UFUNCTION(BlueprintCallable, Category = "MeshLayers")
	bool GetMeshLayerVisible(const FString& LayerName) const;

	/**
	 * @brief Swaps a named mesh layer's materials to the ghost material configured in
	 *        UMeshVisualSettings (bTranslucent=true), or restores its original materials
	 *        (bTranslucent=false). No-op if the layer doesn't exist or no ghost material is set.
	 */
	UFUNCTION(BlueprintCallable, Category = "MeshLayers")
	void SetMeshLayerTranslucent(const FString& LayerName, bool bTranslucent);

	/** @brief Returns whether a named mesh layer currently has the ghost material applied. */
	UFUNCTION(BlueprintCallable, Category = "MeshLayers")
	bool GetMeshLayerTranslucent(const FString& LayerName) const;

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
	TSharedPtr<FJsonObject> GetRawJsonObject() const { return RawJson; }

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

	/** @brief Returns true if this actor's world position currently projects inside the player's viewport bounds. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Entity")
	bool IsInCameraView() const;

	/** @brief Returns true if an entity-detail window is currently open for this actor. */
	bool HasOpenWindow() const { return bHasOpenWindow; }

	/** @brief Sets whether an entity-detail window is open for this actor. Called by EntityWindowWidget on bind/unbind. */
	void SetHasOpenWindow(bool bOpen) { bHasOpenWindow = bOpen; }

	/**
	 * @brief Reads a string value from RawJson by dot-separated path (e.g. "attributes.matricula").
	 * Returns an empty string if the path doesn't exist or the value isn't a string.
	 */
	UFUNCTION(BlueprintCallable, Category = "Entity")
	FString GetRawJsonField(const FString &DotPath) const;

	/**
	 * @brief Like GetRawJsonField but works for any JSON value type (string, number, bool, null).
	 * Numbers are formatted as integers when whole, otherwise to 4 decimal places.
	 */
	UFUNCTION(BlueprintCallable, Category = "Entity")
	FString GetRawJsonFieldAny(const FString &DotPath) const;

	// ---- Mesh layer management (also usable from factory) ----

	/** @brief Creates or replaces a named UStaticMeshComponent layer on this actor at the identity transform. */
	UFUNCTION(BlueprintCallable, Category = "MeshLayers")
	UStaticMeshComponent* AddOrReplaceMeshLayer(const FString& LayerName, UStaticMesh* Mesh);

	/** @brief Same as AddOrReplaceMeshLayer, but placed at RelativeTransform instead of the identity transform. */
	UStaticMeshComponent* AddOrReplaceMeshLayerAt(const FString& LayerName, UStaticMesh* Mesh, const FTransform& RelativeTransform);

	/**
	 * @brief Replaces every mesh layer belonging to GroupName (e.g. "Cone", "Simulation", "Polygon")
	 * with one layer per entry in Layers — named GroupName if there's exactly one, or
	 * "GroupName_<NodeName>" per layer otherwise (splitting a multi-mesh GLB into independently
	 * toggleable layers instead of merging it into a single mesh). Usable by behavior components
	 * that load their own multi-model GLBs (see UEntityBehaviorComponent::HandlesOwnModelLoading).
	 */
	UFUNCTION(BlueprintCallable, Category = "MeshLayers")
	void AddOrReplaceMeshLayerGroup(const FString& GroupName, const TArray<FGlbMeshLayer>& GlbLayers);

	/** @brief Sets the visibility of every layer belonging to GroupName. No-op if the group is empty. */
	UFUNCTION(BlueprintCallable, Category = "MeshLayers")
	void SetLayerGroupVisible(const FString& GroupName, bool bVisible);

	/** @brief True if any layer belonging to GroupName is currently visible. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MeshLayers")
	bool IsLayerGroupVisible(const FString& GroupName) const;

	/** @brief True if GroupName currently has at least one mesh layer. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MeshLayers")
	bool HasLayerGroup(const FString& GroupName) const;

	/**
	 * @brief Re-traces and re-applies the terrain-exclusion polygon (see SpawnTerrainExclusionPolygon).
	 * Safe to call any time the actor's visible mesh/shape has changed — it removes any existing
	 * polygon first. No-op if the actor's current mesh footprint is too small to warrant exclusion.
	 */
	UFUNCTION(BlueprintCallable, Category = "Entity")
	void RebuildTerrainExclusionPolygon() { SpawnTerrainExclusionPolygon(); }

	/** @brief True if a terrain-exclusion polygon is currently active for this actor. */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Entity")
	bool HasTerrainExclusionPolygon() const { return TerrainExclusionPolygon != nullptr; }

private:
	/** @brief Maps a logical group name (e.g. "Simulation") to the actual per-node MeshLayers keys created for it. */
	TMap<FString, TArray<FString>> MeshLayerGroups;

	/** @brief The Ditto thingId string (e.g. "tolls:toll-1"), extracted during Initialize(). */
	FString ThingId;

	/** @brief Original materials of each mesh layer, cached the first time it's made translucent so they can be restored. */
	UPROPERTY()
	TMap<FString, FMeshLayerMaterialSet> OriginalLayerMaterials;

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

	/** @brief Total number of Ditto update messages received for this entity since spawn. */
	int64 ReceivedMessageCount = 0;

	/** @brief World time (seconds) of the most recently received update, or 0 before the first one. */
	double LastMessageReceivedTime = 0.0;

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

	/** @brief Forwards to BehaviorComponent->OnEntityDataChanged() if a behavior component is attached. No-op otherwise. */
	void NotifyBehaviorDataChanged();

	/**
	 * @brief Reads attributes.polygon from RawJson and, if it is a new URL, asks
	 * UGlbModelService to load (or serve from cache) the mesh and apply it.
	 */
	void TryLoadGlbModel();

	/** @brief Reads attributes.polygon from StructInstance's compiled-in default (used when Ditto omits the field). */
	FString GetDefaultPolygonUrl() const;

	/** @brief Callback from UGlbModelService — applies the loaded mesh(es) as the "Polygon" layer group. */
	UFUNCTION()
	void OnPolygonGlbLayersLoaded(const TArray<FGlbMeshLayer>& GlbLayers);

	/** @brief Last polygon URL successfully requested, used to skip redundant reloads. */
	FString LoadedPolygonUrl;

	/** @brief CartographicPolygon that tells Cesium to skip terrain tiles under this actor's GLB model. Null when no GLB is loaded. */
	UPROPERTY()
	ACesiumCartographicPolygon* TerrainExclusionPolygon = nullptr;

	/** @brief Spawns a rectangular CartographicPolygon around LastLatitude/LastLongitude and registers it with the terrain's PolygonRasterOverlay. */
	void SpawnTerrainExclusionPolygon();

	/** @brief Minimum horizontal mesh footprint (cm) before terrain gets excluded under it; smaller models (streetlights, signs) just sit on the terrain as-is. */
	static constexpr float MinExclusionFootprintCm = 1000.f;

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

	/**
	 * @brief True once ApplyScale() has actually resized InteractionBox to match a real
	 * vehicle bounding box (from attributes.length/width/height). Only then does
	 * InteractionBox's height correspond to the center-pivoted default-cube mesh, making
	 * the "+half box height" ground-snap correction in SnapToGround()/OnGroundHeightSampled()
	 * valid. Entities that never get scaled (signs, poles, cameras — no dimensions in their
	 * payload) keep whatever mesh a Blueprint subclass assigned, which is typically already
	 * bottom-pivoted at the actor origin; applying that same offset to them floated them
	 * ~3m above the ground (InteractionBox's untouched 300 UU default half-extent).
	 */
	bool bHasAppliedScale = false;

	/** @brief One-shot timer that fires when expiry_ts is reached. */
	FTimerHandle ExpiryTimer;

	/**
	 * @brief Traces (or, if tiles aren't loaded, async-samples via Cesium) the ground
	 * height/pitch AT THE GIVEN geographic position — not necessarily where the actor
	 * currently is. Only records LastSnappedAltitudeMeters/LastSnappedPitchDeg; never
	 * moves the actor. Called with the car's *current* visual position for the initial
	 * cold-start snap (CheckVisibility), and with the *new* server target position on
	 * every live update thereafter (TriggerSnapIfNeeded) — the latter is what lets
	 * Tick() treat height/pitch as genuine interpolation targets that arrive in lockstep
	 * with lat/lon, instead of a stale value requiring separate dead-reckoning.
	 */
	void SnapToGround(double TraceLatitude, double TraceLongitude);

	/** @brief True if Lat/Lon is within camera-relevant range (reused by CheckVisibility and TriggerSnapIfNeeded). */
	bool IsWithinSnapRange(double Lat, double Lon) const;

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

	/** @brief True while an EntityWindowWidget is open and bound to this actor. Set/cleared by EntityWindowWidget. */
	UPROPERTY()
	bool bHasOpenWindow = false;

	bool bSnappedToGround = false;
	bool bSnapInProgress = false;
	double LastSnappedAltitudeMeters = 0.0;

	/**
	 * @brief Max height difference (metres) between two consecutive snaps for the second
	 * one to count as "converged" — see SnapToGround()/OnGroundHeightSampled(). Below this
	 * the periodic CheckVisibility timer stops; above it, retracing continues since the
	 * tileset is still streaming in higher-detail geometry at this spot.
	 */
	static constexpr double ConvergenceThresholdMeters = 0.15;

	/**
	 * @brief Raw (un-transformed) UE Z of the last successful center ground-trace hit.
	 * Used as the reference height for picking among multiple stacked hits (e.g. an
	 * overpass deck above the actual road) on the *next* trace — see SnapToGround().
	 */
	double LastSnappedRawZ = 0.0;

	/** @brief Incline pitch (degrees) derived from the front/rear ground traces in SnapToGround(). Target for VisualPitchDeg. */
	double LastSnappedPitchDeg = 0.0;

	/** @brief Pitch actually applied to the actor each Tick — interpolated toward LastSnappedPitchDeg using the same progress fraction `t` as VisualAltitudeMeters/lat-lon (see Tick()). */
	double VisualPitchDeg = 0.0;

	/**
	 * @brief Height (metres) actually applied to the actor each Tick.
	 * Interpolated toward LastSnappedAltitudeMeters/LastExplicitAltitude using the exact
	 * same progress fraction `t` as the lat/lon move each frame (see Tick()) — since
	 * SnapToGround now traces at the car's *destination* on every live update (not its
	 * current position on a separate timer), the height target genuinely corresponds to
	 * where lat/lon is heading, so sharing `t` makes height arrive in lockstep with
	 * position instead of needing separate dead-reckoning/smoothing heuristics.
	 */
	double VisualAltitudeMeters = 0.0;

	void TriggerSnapIfNeeded();

	// Smooth position interpolation
	double VisualLatitude = 0.0;
	double VisualLongitude = 0.0;
	bool bHasInterpolationTarget = false;
	bool bReceivedFirstLiveUpdate = false;
	double LastTargetSetTime = 0.0;
	double EstimatedUpdateInterval = 1.0;

	/**
	 * @brief Start-of-leg snapshot for a true constant-rate linear interpolation, taken
	 * the instant a new (non-teleport) SetMovementTarget arrives. Tick() computes
	 * Alpha = (Now - InterpStartTime) / EstimatedUpdateInterval and lerps from these
	 * values to LastLatitude/LastLongitude/height/pitch. Paced by the actual observed
	 * update cadence (EstimatedUpdateInterval) rather than the payload's reported speed,
	 * so the car always arrives exactly when the next update is expected — no early
	 * arrival-then-freeze, no late arrival-then-snap ("fwoop...stop...fwoop").
	 */
	double InterpStartLat = 0.0;
	double InterpStartLon = 0.0;
	double InterpStartHeight = 0.0;
	double InterpStartPitch = 0.0;
	double InterpStartTime = 0.0;

	bool bHasExplicitAngle = false;
	double LastAngleDeg = 0.0;

	void SetMovementTarget(double Lat, double Lon, double SpeedKmh, bool bTeleport = false);
	void SetMovementTarget(double Lat, double Lon, double SpeedKmh, double AngleDeg, double AccelMs2, bool bTeleport = false);

	virtual void Tick(float DeltaTime) override;

	/**
	 * @brief Reads a string-valued property from the live StructInstance by name.
	 * @param PropertyName Case-insensitive property name to look up.
	 * @return The string value, or an empty FString if not found.
	 */
	FString GetStringProperty(const FString &PropertyName);
};
