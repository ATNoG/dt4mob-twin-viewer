// Fill out your copyright notice in the Description page of Project Settings.

/** @file TempUIActor.cpp
 *  @brief Implementation of ATempUIActor.
 *
 *  The header (TempUIActor.h) documents all public and private members.
 *  This file implements the JSON-routing logic in HandleEntityUpdate() and the
 *  coordinate-extraction loop in SetLocation() which traverses the reflected
 *  struct hierarchy at runtime to find latitude/longitude properties.
 */
#include "Entities/TempUIActor.h"
#include "Entities/MeshVisualSettings.h"
#include "Materials/MaterialInterface.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Services/EntityUpdateDaemon.h"
#include "Services/WSService.h"
#include "Services/GlbModelService.h"
#include "Managers/SelectionManager.h"
#include "Services/CoordinatesConversionService.h"
#include "Services/ActorRegistryService.h"
#include "CesiumSampleHeightMostDetailedAsyncAction.h"
#include "Cesium3DTileset.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "CesiumGeoreference.h"
#include "DrawDebugHelpers.h"
#include "CesiumCartographicPolygon.h"
#include "CesiumPolygonRasterOverlay.h"
#include "Components/SplineComponent.h"
#include "EntityStructs/IgnitionPointStruct.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

// ============================================================
//  Construction
// ============================================================

ATempUIActor::ATempUIActor()
{
	PrimaryActorTick.bCanEverTick = true;

	GlobeAnchor = CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("GlobeAnchor"));

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(SceneRoot);
	InteractionBox->SetBoxExtent(FVector(300.f, 300.f, 300.f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMeshComponent->SetupAttachment(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		StaticMeshComponent->SetStaticMesh(CubeMesh.Object);
	}

	StaticMeshComponent->SetRelativeLocation(FVector(0.f, 0.f, 50.f));
	StaticMeshComponent->SetWorldScale3D(FVector(1.f));
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

// ============================================================
//  BeginPlay
// ============================================================

void ATempUIActor::BeginPlay()
{
	Super::BeginPlay();

	// SnapToGround(); // TODO: re-enable once camera-visibility detection is in place

	GetWorldTimerManager().SetTimer(VisibilityCheckTimer, this, &ATempUIActor::CheckVisibility, 0.2f, true);

	// Selection system
	if (ULocalPlayer *LP = GetWorld()->GetFirstLocalPlayerFromController())
	{
		if (USelectionManager *Mgr = LP->GetSubsystem<USelectionManager>())
		{
			Mgr->OnHoveredActorChanged.AddDynamic(this, &ATempUIActor::HandleHoverChanged);
			Mgr->OnSelectedActorChanged.AddDynamic(this, &ATempUIActor::HandleSelectedChanged);
		}
	}

	// Register with Daemon — ThingId is populated by Initialize()
	if (!ThingId.IsEmpty())
	{
		if (UGameInstance *GI = GetGameInstance())
		{
			if (UEntityUpdateDaemon *Daemon = GI->GetSubsystem<UEntityUpdateDaemon>())
			{
				OnEntityUpdated.AddDynamic(this, &ATempUIActor::HandleEntityUpdate);
				Daemon->RegisterEntity(ThingId, OnEntityUpdated);
			}
		}

		if (UActorRegistryService *Registry = UActorRegistryService::Get(this))
			Registry->RegisterActor(ThingId, this);
	}
}

// ============================================================
//  EndPlay
// ============================================================

void ATempUIActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (!ThingId.IsEmpty())
	{
		if (UGameInstance *GI = GetGameInstance())
		{
			if (UEntityUpdateDaemon *Daemon = GI->GetSubsystem<UEntityUpdateDaemon>())
				Daemon->UnregisterEntity(ThingId, OnEntityUpdated);
		}

		if (UActorRegistryService *Registry = UActorRegistryService::Get(this))
			Registry->UnregisterActor(ThingId);
	}

	if (ULocalPlayer *LP = GetWorld()->GetFirstLocalPlayerFromController())
	{
		if (USelectionManager *Mgr = LP->GetSubsystem<USelectionManager>())
		{
			if (Mgr->GetSelectedActor() == this)
				Mgr->SetSelectedActor(nullptr);

			Mgr->OnHoveredActorChanged.RemoveAll(this);
			Mgr->OnSelectedActorChanged.RemoveAll(this);
		}
	}

	GetWorldTimerManager().ClearTimer(VisibilityCheckTimer);

	RemoveTerrainExclusionPolygon();

	Super::EndPlay(EndPlayReason);
}

// ============================================================
//  Initialize
// ============================================================

void ATempUIActor::Initialize(UScriptStruct *InType, TSharedPtr<FJsonObject> JsonObject)
{
	DataStructType = InType;
	StructInstance = MakeShared<FStructOnScope>(DataStructType);
	RawJson = JsonObject;

	FJsonObjectConverter::JsonObjectToUStruct(
		JsonObject.ToSharedRef(), DataStructType, StructInstance->GetStructMemory(), 0, 0);

	ThingId = GetStringProperty(TEXT("thingId"));
#if WITH_EDITOR
	SetActorLabel(*ThingId);
#endif
	UE_LOG(LogTemp, Log, TEXT("TempUIActor initialized: [%s]"), *ThingId);

	SetLocation();
	TryApplyExpiry();
	RefreshFireExclusion();
	TryLoadGlbModel();

	// Scale mesh from attributes.length/width/height (TRACI and similar sources)
	const TSharedPtr<FJsonObject> *AttrsPtr = nullptr;
	if (JsonObject->TryGetObjectField(TEXT("attributes"), AttrsPtr) && AttrsPtr && (*AttrsPtr).IsValid())
	{
		double L = 0.0, W = 0.0, H = 0.0;
		(*AttrsPtr)->TryGetNumberField(TEXT("length"), L);
		(*AttrsPtr)->TryGetNumberField(TEXT("width"),  W);
		(*AttrsPtr)->TryGetNumberField(TEXT("height"), H);
		if (L > 0.0 || W > 0.0 || H > 0.0)
			ApplyScale(L, W, H);
	}

	// If BeginPlay already ran (mid-play spawn), register now
	if (HasActorBegunPlay() && !ThingId.IsEmpty())
	{
		if (UGameInstance *GI = GetGameInstance())
		{
			if (UEntityUpdateDaemon *Daemon = GI->GetSubsystem<UEntityUpdateDaemon>())
			{
				if (!OnEntityUpdated.IsBound())
				{
					OnEntityUpdated.AddDynamic(this, &ATempUIActor::HandleEntityUpdate);
				}
				Daemon->RegisterEntity(ThingId, OnEntityUpdated);
			}
		}

		if (UActorRegistryService *Registry = UActorRegistryService::Get(this))
			Registry->RegisterActor(ThingId, this);
	}
}

// ============================================================
//  HandleEntityUpdate  — entry point from Daemon
// ============================================================

void ATempUIActor::HandleEntityUpdate(const FString &Path, const FString &ValueJson)
{
	UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: update on path '%s'"), *ThingId, *Path);

	// Track message cadence so a stall (dropped WS message, Ditto backpressure, or a
	// dying socket) shows up as a log error instead of silently manifesting as a car
	// sitting still for a while and then dashing to catch up.
	if (UWorld *World = GetWorld())
	{
		const double Now = World->GetTimeSeconds();
		if (LastMessageReceivedTime > 0.0 && ThingId.Contains(TEXT("traci")))
		{
			// Adaptive rather than a flat constant: EstimatedUpdateInterval is this car's
			// own rolling-average inter-update time (updated in SetMovementTarget), so the
			// threshold tracks whatever cadence the simulation is actually running at
			// instead of assuming a fixed 1 msg/s. Floored at 3s so a car's very first
			// couple of updates (before the average has settled) don't false-trigger.
			const double MaxExpectedGapSeconds = FMath::Max(3.0, EstimatedUpdateInterval * 3.0);
			const double GapSeconds = Now - LastMessageReceivedTime;
			if (GapSeconds > MaxExpectedGapSeconds)
			{
				UGameInstance *GI = GetGameInstance();
				UWSService *WS = GI ? GI->GetSubsystem<UWSService>() : nullptr;
				const bool bSocketConnected = WS && WS->IsConnected();
				const FString Timestamp = FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S.%s"));
				UE_LOG(LogTemp, Error,
					   TEXT("[%s] TempUIActor [%s]: no update for %.2fs (expected ~%.2fs, msg #%lld so far, socket %s) — ")
					   TEXT("possible dropped message, Ditto backpressure, or socket stall"),
					   *Timestamp, *ThingId, GapSeconds, MaxExpectedGapSeconds, ReceivedMessageCount, bSocketConnected ? TEXT("connected") : TEXT("DISCONNECTED"));
			}
		}
		LastMessageReceivedTime = Now;
	}
	++ReceivedMessageCount;

	if (ValueJson.IsEmpty() || ValueJson == TEXT("{}"))
	{
		return;
	}

	// Parse the value JSON — may be a JSON object or a primitive (string, number, bool)
	TSharedPtr<FJsonObject> ValueObject;
	{
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ValueJson);
		FJsonSerializer::Deserialize(Reader, ValueObject);
	}

	if (!ValueObject.IsValid())
	{
		// Primitive leaf value (string, number, bool) — wrap into a temporary object to extract it
		FString Wrapped = TEXT("{\"_v\":") + ValueJson + TEXT("}");
		TSharedPtr<FJsonObject> Wrapper;
		TSharedRef<TJsonReader<>> WrapReader = TJsonReaderFactory<>::Create(Wrapped);
		if (!FJsonSerializer::Deserialize(WrapReader, Wrapper) || !Wrapper.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("TempUIActor [%s]: failed to parse value JSON for path '%s'"),
				   *ThingId, *Path);
			return;
		}
		TSharedPtr<FJsonValue> LeafValue = Wrapper->TryGetField(TEXT("_v"));
		if (!LeafValue.IsValid())
		{
			return;
		}
		ApplyLeafPatch(Path, LeafValue);
		return;
	}

	// ---- Route by Ditto path ----
	//
	//  Path examples:
	//    "/"                                       → full thing replace
	//    "/attributes"                             → all attributes replaced
	//    "/attributes/location"                    → single attribute
	//    "/features"                               → all features replaced
	//    "/features/lidar-outsight"                → one feature replaced
	//    "/features/lidar-outsight/properties"     → all properties of a feature
	//    "/features/lidar-outsight/properties/1"   → one property entry
	//

	if (Path == TEXT("/") || Path.StartsWith(TEXT("/attributes")))
	{
		// Full-thing replace or attribute patch — merge into top-level RawJson.
		ApplyFullOrAttributePatch(ValueObject, Path);
		return;
	}

	if (Path.StartsWith(TEXT("/features")))
	{
		// Parse: /features/<featureName>/properties/<entryId>
		// Split on '/' — produces ["", "features", "<featureName>", "properties", "<entryId>"]
		TArray<FString> Segments;
		Path.ParseIntoArray(Segments, TEXT("/"), /*CullEmpty=*/true);

		// Segments[0] = "features"
		// Segments[1] = featureName  (e.g. "lidar-outsight")
		// Segments[2] = "properties"     (optional)
		// Segments[3] = entryId          (optional, e.g. "1")

		const FString FeatureName = Segments.IsValidIndex(1) ? Segments[1] : TEXT("");
		const FString EntryId = Segments.IsValidIndex(3) ? Segments[3] : TEXT("");

		if (FeatureName.IsEmpty())
		{
			// "/features" alone — treat as full features replace
			ApplyFullOrAttributePatch(ValueObject, Path);
			return;
		}

		if (!EntryId.IsEmpty())
		{
			// Most common live path: "/features/<name>/properties/<id>"
			ApplyFeaturePatch(FeatureName, EntryId, ValueObject);
		}
		else if (Segments.Num() == 3 && Segments[2].Equals(TEXT("properties"), ESearchCase::IgnoreCase))
		{
			// "/features/<name>/properties" — whole properties block replaced (e.g. camera-ORT State)
			ApplyFeaturePropertiesPatch(FeatureName, ValueObject);
		}
		else
		{
			// "/features/<name>" — merge at feature level
			ApplyFullOrAttributePatch(ValueObject, Path);
		}

		return;
	}

	// Unrecognised path — log and ignore
	UE_LOG(LogTemp, Warning, TEXT("TempUIActor [%s]: unhandled path '%s'"), *ThingId, *Path);
}

// ============================================================
//  DeepMergeJsonObjects
//  Recursively merges Source into Target. Nested objects are merged;
//  all other value types replace the target field outright.
// ============================================================

static void DeepMergeJsonObjects(TSharedPtr<FJsonObject> Target, TSharedPtr<FJsonObject> Source)
{
	for (const auto &Field : Source->Values)
	{
		const TSharedPtr<FJsonValue> *Existing = Target->Values.Find(Field.Key);
		if (Existing && (*Existing)->Type == EJson::Object && Field.Value->Type == EJson::Object)
			DeepMergeJsonObjects((*Existing)->AsObject(), Field.Value->AsObject());
		else
			Target->SetField(Field.Key, Field.Value);
	}
}

// ============================================================
//  ApplyFullOrAttributePatch
//  For "/" and "/attributes/..." paths — merge into top-level RawJson
// ============================================================

void ATempUIActor::ApplyFullOrAttributePatch(TSharedPtr<FJsonObject> ValueObject,
											 const FString &Path)
{
	if (!ValueObject.IsValid())
		return;

	if (!RawJson.IsValid())
	{
		RawJson = ValueObject;
	}
	else
	{
		// Deep merge: nested objects are merged recursively so that fields absent
		// from the patch (e.g. thingId, policyId, matricula) are never lost.
		DeepMergeJsonObjects(RawJson, ValueObject);
	}

	// Re-deserialise so struct accessors and SetLocation() stay current
	if (StructInstance.IsValid() && DataStructType)
	{
		FJsonObjectConverter::JsonObjectToUStruct(
			RawJson.ToSharedRef(), DataStructType, StructInstance->GetStructMemory(), 0, 0);
	}

	// A location change may have arrived — re-evaluate position
	const bool bLocationPath = Path == TEXT("/") ||
							   Path.Contains(TEXT("location")) ||
							   Path.Contains(TEXT("geometry")) ||
							   Path.Contains(TEXT("attributes"));
	if (bLocationPath)
	{
		SetLocation();
		TryApplyExpiry();
	}

	OnEntityDataChanged.Broadcast();
	RefreshFireExclusion();
	TryLoadGlbModel();
}

// ============================================================
//  ApplyLeafPatch
//  For primitive values at a leaf path, e.g. "/attributes/expiry_ts" = "2026-..."
// ============================================================

void ATempUIActor::ApplyLeafPatch(const FString &Path, TSharedPtr<FJsonValue> Value)
{
	if (!RawJson.IsValid() || !Value.IsValid())
		return;

	TArray<FString> Segments;
	Path.ParseIntoArray(Segments, TEXT("/"), /*CullEmpty=*/true);
	if (Segments.IsEmpty())
		return;

	const FString LeafKey = Segments.Last();

	// Navigate to the parent object
	TSharedPtr<FJsonObject> Current = RawJson;
	for (int32 i = 0; i < Segments.Num() - 1; i++)
	{
		const TSharedPtr<FJsonObject> *Next = nullptr;
		if (!Current->TryGetObjectField(Segments[i], Next) || !Next)
			return;
		Current = *Next;
	}

	Current->SetField(LeafKey, Value);

	if (StructInstance.IsValid() && DataStructType)
	{
		FJsonObjectConverter::JsonObjectToUStruct(
			RawJson.ToSharedRef(), DataStructType, StructInstance->GetStructMemory(), 0, 0);
	}

	OnEntityDataChanged.Broadcast();

	if (LeafKey.Equals(TEXT("expiry_ts"), ESearchCase::IgnoreCase))
		TryApplyExpiry();
}

// ============================================================
//  ApplyFeaturePatch
//  For "/features/<featureName>/properties/<entryId>" paths
//
//  Live example:
//    path  = "/features/lidar-outsight/properties/1"
//    value = {
//      "id": "1",
//      "ts": "2026-03-09T18:10:48.068971Z",
//      "extra": {
//        "timeOfMeasurement": "...",
//        "speedKmh": 2.0,
//        "localCoordinates": { "x": 5.857, "y": 4.859 }
//      }
//    }
//
//  We write the value into:
//    RawJson["features"][featureName]["properties"][entryId]
//  and then do any derived updates (speed display, local-coordinate
//  conversion, etc.) directly from the value object.
// ============================================================

void ATempUIActor::ApplyFeaturePatch(const FString &FeatureName,
									 const FString &EntryId,
									 TSharedPtr<FJsonObject> ValueObject)
{
	if (!ValueObject.IsValid())
		return;

	// ---- Keep RawJson in sync ----
	if (RawJson.IsValid())
	{
		// Navigate / create: RawJson → "features" → featureName → "properties"
		TSharedPtr<FJsonObject> FeaturesObj;
		{
			const TSharedPtr<FJsonObject> *Tmp = nullptr;
			if (RawJson->TryGetObjectField(TEXT("features"), Tmp) && Tmp)
				FeaturesObj = *Tmp;
			else
				FeaturesObj = MakeShared<FJsonObject>();
		}

		TSharedPtr<FJsonObject> ThisFeatureObj;
		{
			const TSharedPtr<FJsonObject> *Tmp = nullptr;
			if (FeaturesObj->TryGetObjectField(FeatureName, Tmp) && Tmp)
				ThisFeatureObj = *Tmp;
			else
				ThisFeatureObj = MakeShared<FJsonObject>();
		}

		TSharedPtr<FJsonObject> PropertiesObj;
		{
			const TSharedPtr<FJsonObject> *Tmp = nullptr;
			if (ThisFeatureObj->TryGetObjectField(TEXT("properties"), Tmp) && Tmp)
				PropertiesObj = *Tmp;
			else
				PropertiesObj = MakeShared<FJsonObject>();
		}

		// Write the new entry
		PropertiesObj->SetObjectField(EntryId, ValueObject);
		ThisFeatureObj->SetObjectField(TEXT("properties"), PropertiesObj);
		FeaturesObj->SetObjectField(FeatureName, ThisFeatureObj);
		RawJson->SetObjectField(TEXT("features"), FeaturesObj);
	}

	OnEntityDataChanged.Broadcast();

	// ---- Derived updates from the live entry ----
	//
	//  Pull out the "extra" sub-object which carries the measurement payload.
	//  Other features or future schemas may not have "extra" — guard carefully.

	const TSharedPtr<FJsonObject> *ExtraPtr = nullptr;
	if (!ValueObject->TryGetObjectField(TEXT("extra"), ExtraPtr) || !ExtraPtr || !(*ExtraPtr).IsValid())
	{
		// No extra block — nothing visual to update for this entry
		return;
	}

	const TSharedPtr<FJsonObject> &Extra = *ExtraPtr;

	// -- Speed --
	double SpeedKmh = 0.0;
	if (Extra->TryGetNumberField(TEXT("speedKmh"), SpeedKmh))
	{
		UE_LOG(LogTemp, Verbose, TEXT("TempUIActor [%s]: speedKmh = %.2f"), *ThingId, SpeedKmh);
		// TODO: push to widget / material parameter
	}

	// -- Local coordinates (sensor frame, not geo) --
	const TSharedPtr<FJsonObject> *LocalCoordsPtr = nullptr;
	if (Extra->TryGetObjectField(TEXT("localCoordinates"), LocalCoordsPtr) && LocalCoordsPtr && (*LocalCoordsPtr).IsValid())
	{
		double LocalX = 0.0, LocalY = 0.0;
		(*LocalCoordsPtr)->TryGetNumberField(TEXT("x"), LocalX);
		(*LocalCoordsPtr)->TryGetNumberField(TEXT("y"), LocalY);
		UE_LOG(LogTemp, Verbose, TEXT("TempUIActor [%s]: localCoords = (%.3f, %.3f)"),
			   *ThingId, LocalX, LocalY);
		// TODO: convert sensor-local → world if calibration matrix is available
	}

	// -- Timestamp --
	FString Timestamp;
	if (Extra->TryGetStringField(TEXT("timeOfMeasurement"), Timestamp))
	{
		UE_LOG(LogTemp, Verbose, TEXT("TempUIActor [%s]: timeOfMeasurement = %s"),
			   *ThingId, *Timestamp);
	}

	// -- Absolute coordinates → reposition actor --
	const TSharedPtr<FJsonObject> *AbsCoordsPtr = nullptr;
	if (Extra->TryGetObjectField(TEXT("absoluteCoordinates"), AbsCoordsPtr) &&
		AbsCoordsPtr && (*AbsCoordsPtr).IsValid())
	{
		double Lat = 0.0, Lon = 0.0;
		(*AbsCoordsPtr)->TryGetNumberField(TEXT("latitude"),  Lat);
		(*AbsCoordsPtr)->TryGetNumberField(TEXT("longitude"), Lon);
		if (Lat != 0.0 || Lon != 0.0)
		{
			SetMovementTarget(Lat, Lon, SpeedKmh, !bReceivedFirstLiveUpdate);
			bReceivedFirstLiveUpdate = true;
			UE_LOG(LogTemp, Verbose, TEXT("TempUIActor [%s]: absoluteCoordinates (%.6f, %.6f)"),
				   *ThingId, Lat, Lon);
		}
	}

	// -- Dimensions → scale mesh to bounding box --
	const TSharedPtr<FJsonObject> *DimsPtr = nullptr;
	if (Extra->TryGetObjectField(TEXT("dimensions"), DimsPtr) &&
		DimsPtr && (*DimsPtr).IsValid())
	{
		double W = 0.0, L = 0.0, H = 0.0;
		(*DimsPtr)->TryGetNumberField(TEXT("width"),  W);
		(*DimsPtr)->TryGetNumberField(TEXT("length"), L);
		(*DimsPtr)->TryGetNumberField(TEXT("height"), H);
		if (W > 0.0 || L > 0.0 || H > 0.0)
			ApplyScale(L, W, H);
	}
}

// ============================================================
//  ApplyFeaturePropertiesPatch
//  For "/features/<name>/properties" paths — whole properties block replaced.
//  Used by camera-ORT State updates which carry absoluteCoordinates at the root.
// ============================================================

void ATempUIActor::ApplyFeaturePropertiesPatch(const FString &FeatureName, TSharedPtr<FJsonObject> ValueObject)
{
	if (!ValueObject.IsValid())
		return;

	// Update RawJson["features"][FeatureName]["properties"] = ValueObject
	if (RawJson.IsValid())
	{
		TSharedPtr<FJsonObject> FeaturesObj;
		{
			const TSharedPtr<FJsonObject> *Tmp = nullptr;
			FeaturesObj = (RawJson->TryGetObjectField(TEXT("features"), Tmp) && Tmp)
				? *Tmp : MakeShared<FJsonObject>();
		}

		TSharedPtr<FJsonObject> ThisFeatureObj;
		{
			const TSharedPtr<FJsonObject> *Tmp = nullptr;
			ThisFeatureObj = (FeaturesObj->TryGetObjectField(FeatureName, Tmp) && Tmp)
				? *Tmp : MakeShared<FJsonObject>();
		}

		ThisFeatureObj->SetObjectField(TEXT("properties"), ValueObject);
		FeaturesObj->SetObjectField(FeatureName, ThisFeatureObj);
		RawJson->SetObjectField(TEXT("features"), FeaturesObj);
	}

	if (StructInstance.IsValid() && DataStructType)
	{
		FJsonObjectConverter::JsonObjectToUStruct(
			RawJson.ToSharedRef(), DataStructType, StructInstance->GetStructMemory(), 0, 0);
	}

	// Reposition from absoluteCoordinates (at value root, not under "extra")
	const TSharedPtr<FJsonObject> *AbsCoordsPtr = nullptr;
	if (ValueObject->TryGetObjectField(TEXT("absoluteCoordinates"), AbsCoordsPtr) &&
		AbsCoordsPtr && (*AbsCoordsPtr).IsValid())
	{
		double Lat = 0.0, Lon = 0.0;
		(*AbsCoordsPtr)->TryGetNumberField(TEXT("latitude"),  Lat);
		(*AbsCoordsPtr)->TryGetNumberField(TEXT("longitude"), Lon);
		if (Lat != 0.0 || Lon != 0.0)
		{
			SetMovementTarget(Lat, Lon, 0.0, !bReceivedFirstLiveUpdate);
			bReceivedFirstLiveUpdate = true;
		}
	}

	// Scale mesh from dimensions
	const TSharedPtr<FJsonObject> *DimsPtr = nullptr;
	if (ValueObject->TryGetObjectField(TEXT("dimensions"), DimsPtr) &&
		DimsPtr && (*DimsPtr).IsValid())
	{
		double W = 0.0, L = 0.0, H = 0.0;
		(*DimsPtr)->TryGetNumberField(TEXT("width"),  W);
		(*DimsPtr)->TryGetNumberField(TEXT("length"), L);
		(*DimsPtr)->TryGetNumberField(TEXT("height"), H);
		if (W > 0.0 || L > 0.0 || H > 0.0)
			ApplyScale(L, W, H);
	}

	// TRACI style: direct latitude/longitude at properties root (no sub-object)
	double DirectLat = 0.0, DirectLon = 0.0, DirectSpeedMs = 0.0, DirectAngle = 0.0, DirectAccel = 0.0;
	ValueObject->TryGetNumberField(TEXT("latitude"),  DirectLat);
	ValueObject->TryGetNumberField(TEXT("longitude"), DirectLon);
	ValueObject->TryGetNumberField(TEXT("speed"),     DirectSpeedMs);
	const bool bHasAngle = ValueObject->TryGetNumberField(TEXT("angle"), DirectAngle);
	ValueObject->TryGetNumberField(TEXT("accel"),     DirectAccel);
	if (DirectLat != 0.0 || DirectLon != 0.0)
	{
		if (bHasAngle)
			SetMovementTarget(DirectLat, DirectLon, DirectSpeedMs * 3.6, DirectAngle, DirectAccel, !bReceivedFirstLiveUpdate);
		else
			SetMovementTarget(DirectLat, DirectLon, DirectSpeedMs * 3.6, !bReceivedFirstLiveUpdate);
		bReceivedFirstLiveUpdate = true;
	}

	OnEntityDataChanged.Broadcast();

	// Re-fetch fire perimeter GeoJSONs when cone or perimeters feature is patched via WebSocket.
	if (DataStructType == FIgnitionPointData::StaticStruct() &&
		(FeatureName == TEXT("cone") || FeatureName == TEXT("perimeters")))
	{
		TryFetchFirePerimeters();
	}
}

// ============================================================
//  Highlight
// ============================================================

void ATempUIActor::UpdateHighlight()
{
	bool bEnable = IsHovered || IsSelected;
	TArray<UPrimitiveComponent *> Components;
	GetComponents<UPrimitiveComponent>(Components);

	for (UPrimitiveComponent *Comp : Components)
	{
		Comp->SetRenderCustomDepth(bEnable);
		if (!bEnable)
			continue;
		Comp->SetCustomDepthStencilValue(IsSelected ? 2 : 1);
	}
}

void ATempUIActor::HandleHoverChanged(AActor *NewHoveredActor)
{
	IsHovered = (NewHoveredActor == this);
	UpdateHighlight();
}

void ATempUIActor::HandleSelectedChanged(AActor *NewSelectedActor)
{
	IsSelected = (NewSelectedActor == this);
	UpdateHighlight();
}

// ============================================================
//  Helpers
// ============================================================

FString ATempUIActor::GetJsonString() const
{
	if (!RawJson.IsValid())
		return FString();
	FString Out;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Out);
	FJsonSerializer::Serialize(RawJson.ToSharedRef(), Writer);
	return Out;
}

FString ATempUIActor::GetStringProperty(const FString &PropertyName)
{
	if (!StructInstance.IsValid() || !DataStructType)
		return FString();

	for (TFieldIterator<FProperty> It(DataStructType); It; ++It)
	{
		if (!It->GetName().Equals(PropertyName, ESearchCase::IgnoreCase))
			continue;
		void *Val = It->ContainerPtrToValuePtr<void>(StructInstance->GetStructMemory());
		if (FStrProperty *P = CastField<FStrProperty>(*It))
			return *(FString *)Val;
		if (FNameProperty *P = CastField<FNameProperty>(*It))
			return ((FName *)Val)->ToString();
		break;
	}
	return FString();
}

FString ATempUIActor::GetRawJsonField(const FString &DotPath) const
{
	if (!RawJson.IsValid())
		return FString();

	TArray<FString> Keys;
	DotPath.ParseIntoArray(Keys, TEXT("."), true);

	const TSharedPtr<FJsonObject> *Current = &RawJson;
	for (int32 i = 0; i < Keys.Num() - 1; ++i)
	{
		const TSharedPtr<FJsonObject> *Next = nullptr;
		if (!(*Current)->TryGetObjectField(Keys[i], Next))
			return FString();
		Current = Next;
	}

	FString Result;
	(*Current)->TryGetStringField(Keys.Last(), Result);
	return Result;
}

FString ATempUIActor::GetRawJsonFieldAny(const FString &DotPath) const
{
	if (!RawJson.IsValid())
		return FString();

	TArray<FString> Keys;
	DotPath.ParseIntoArray(Keys, TEXT("."), true);

	const TSharedPtr<FJsonObject> *Current = &RawJson;
	for (int32 i = 0; i < Keys.Num() - 1; ++i)
	{
		const TSharedPtr<FJsonObject> *Next = nullptr;
		if (!(*Current)->TryGetObjectField(Keys[i], Next))
			return FString();
		Current = Next;
	}

	const TSharedPtr<FJsonValue> *ValuePtr = (*Current)->Values.Find(Keys.Last());
	if (!ValuePtr || !ValuePtr->IsValid())
		return FString();

	switch ((*ValuePtr)->Type)
	{
		case EJson::String:  return (*ValuePtr)->AsString();
		case EJson::Boolean: return (*ValuePtr)->AsBool() ? TEXT("true") : TEXT("false");
		case EJson::Null:    return TEXT("null");
		case EJson::Number:
		{
			const double Num = (*ValuePtr)->AsNumber();
			return (Num == FMath::FloorToDouble(Num))
				? FString::Printf(TEXT("%.0f"), Num)
				: FString::Printf(TEXT("%.4f"), Num);
		}
		default: return FString();
	}
}

// ============================================================
//  RefreshFireExclusion — respawn exclusion polygon from fire perimeter data
// ============================================================

void ATempUIActor::RefreshFireExclusion()
{
	if (!StructInstance.IsValid() || DataStructType != FIgnitionPointData::StaticStruct())
		return;

	const FIgnitionPointData *Data =
		reinterpret_cast<const FIgnitionPointData *>(StructInstance->GetStructMemory());

	// Kick off any GeoJSON fetches for URLs that have arrived since last call.
	if (!Data->features.cone.properties.perimeters.IsEmpty() ||
		!Data->features.perimeters.properties.perimeters.IsEmpty())
	{
		TryFetchFirePerimeters();
	}

	// If we already have parsed points and an active exclusion polygon, rebuild it.
	if (TerrainExclusionPolygon &&
		(!ParsedConePerimeterPoints.IsEmpty() || !ParsedPerimeterSteps.IsEmpty()))
	{
		RemoveTerrainExclusionPolygon();
		SpawnTerrainExclusionPolygon();
	}
}

void ATempUIActor::SetLocation()
{
	if (!StructInstance.IsValid() || !DataStructType)
		return;

	FVector Location(0.f, 0.f, 0.f);
	bHasExplicitAltitude = false;

	for (TFieldIterator<FProperty> It(DataStructType); It; ++It)
	{
		if (!It->GetName().Equals(TEXT("attributes"), ESearchCase::IgnoreCase))
			continue;

		FStructProperty *StructProp = CastField<FStructProperty>(*It);
		if (!StructProp)
			continue;

		void *AttribPtr = StructProp->ContainerPtrToValuePtr<void>(StructInstance->GetStructMemory());
		UScriptStruct *AttribStruct = StructProp->Struct;

		for (TFieldIterator<FProperty> AttrIt(AttribStruct); AttrIt; ++AttrIt)
		{
			const FString AttrName = AttrIt->GetName();

			if (AttrName.Equals(TEXT("location"), ESearchCase::IgnoreCase) ||
				AttrName.Equals(TEXT("geometry"), ESearchCase::IgnoreCase) ||
				AttrName.Equals(TEXT("coordinates"), ESearchCase::IgnoreCase))
			{
				FStructProperty *LocProp = CastField<FStructProperty>(*AttrIt);
				if (!LocProp)
					continue;

				void *LocPtr = LocProp->ContainerPtrToValuePtr<void>(AttribPtr);
				UScriptStruct *LocStruct = LocProp->Struct;

				for (TFieldIterator<FProperty> LocIt(LocStruct); LocIt; ++LocIt)
				{
					void *V = LocIt->ContainerPtrToValuePtr<void>(LocPtr);
					const FString LocName = LocIt->GetName();
					if (LocName.Equals(TEXT("latitude"), ESearchCase::IgnoreCase) ||
						LocName.Equals(TEXT("lat"), ESearchCase::IgnoreCase))
					{
						if (FDoubleProperty *P = CastField<FDoubleProperty>(*LocIt))
							Location.X = *(double *)V;
					}
					else if (LocName.Equals(TEXT("longitude"), ESearchCase::IgnoreCase) ||
							 LocName.Equals(TEXT("lon"), ESearchCase::IgnoreCase))
					{
						if (FDoubleProperty *P = CastField<FDoubleProperty>(*LocIt))
							Location.Y = *(double *)V;
					}
					else if (LocName.Equals(TEXT("altitude"), ESearchCase::IgnoreCase) ||
							 LocName.Equals(TEXT("alt"), ESearchCase::IgnoreCase))
					{
						if (FDoubleProperty *P = CastField<FDoubleProperty>(*LocIt))
							Location.Z = *(double *)V;
					}
				}
				if (Location.Z != 0.0)
				{
					// EGM96 geoid undulation for Portugal (~39°N, 9°W): converts MSL → WGS-84 ellipsoid.
					constexpr double GeoidUndulationMeters = 53.0;
					bHasExplicitAltitude = true;
					LastExplicitAltitude = Location.Z + GeoidUndulationMeters;
				}
				break;
			}

			if (AttrName.Equals(TEXT("latitude"), ESearchCase::IgnoreCase) ||
				AttrName.Equals(TEXT("lat"), ESearchCase::IgnoreCase))
			{
				if (FDoubleProperty *P = CastField<FDoubleProperty>(*AttrIt))
					Location.X = *(double *)P->ContainerPtrToValuePtr<void>(AttribPtr);
			}
			else if (AttrName.Equals(TEXT("longitude"), ESearchCase::IgnoreCase) ||
					 AttrName.Equals(TEXT("lon"), ESearchCase::IgnoreCase))
			{
				if (FDoubleProperty *P = CastField<FDoubleProperty>(*AttrIt))
					Location.Y = *(double *)P->ContainerPtrToValuePtr<void>(AttribPtr);
			}
		}

		// Generic fallback: scan every struct sub-property of attributes for lat/lon variants.
		// Handles e.g. FIgnitionPointAttributes::fire_ignition.lat/lon.
		if (Location.X == 0.0 && Location.Y == 0.0)
		{
			for (TFieldIterator<FProperty> AttrIt(AttribStruct); AttrIt; ++AttrIt)
			{
				FStructProperty *SubProp = CastField<FStructProperty>(*AttrIt);
				if (!SubProp)
					continue;

				void *SubPtr = SubProp->ContainerPtrToValuePtr<void>(AttribPtr);
				UScriptStruct *SubStruct = SubProp->Struct;

				for (TFieldIterator<FProperty> SubIt(SubStruct); SubIt; ++SubIt)
				{
					const FString SubName = SubIt->GetName();
					void *V = SubIt->ContainerPtrToValuePtr<void>(SubPtr);

					if (SubName.Equals(TEXT("latitude"), ESearchCase::IgnoreCase) ||
						SubName.Equals(TEXT("lat"), ESearchCase::IgnoreCase))
					{
						if (FDoubleProperty *P = CastField<FDoubleProperty>(*SubIt))
							Location.X = *(double *)V;
					}
					else if (SubName.Equals(TEXT("longitude"), ESearchCase::IgnoreCase) ||
							 SubName.Equals(TEXT("lon"), ESearchCase::IgnoreCase))
					{
						if (FDoubleProperty *P = CastField<FDoubleProperty>(*SubIt))
							Location.Y = *(double *)V;
					}
				}

				if (Location.X != 0.0 || Location.Y != 0.0)
					break;
			}
		}
	}

	// Last-resort fallback: scan RawJson features for absoluteCoordinates.
	// Handles entities (e.g. FTollCameraData) whose coordinates live in
	// features.<name>.properties.absoluteCoordinates rather than attributes.
	if (Location.X == 0.0 && Location.Y == 0.0 && RawJson.IsValid())
	{
		const TSharedPtr<FJsonObject> *FeaturesPtr = nullptr;
		if (RawJson->TryGetObjectField(TEXT("features"), FeaturesPtr) && FeaturesPtr)
		{
			for (const auto &FeaturePair : (*FeaturesPtr)->Values)
			{
				const TSharedPtr<FJsonObject> *FeatureObj = nullptr;
				if (!(*FeaturesPtr)->TryGetObjectField(FeaturePair.Key, FeatureObj) || !FeatureObj)
					continue;

				const TSharedPtr<FJsonObject> *PropsObj = nullptr;
				if (!(*FeatureObj)->TryGetObjectField(TEXT("properties"), PropsObj) || !PropsObj)
					continue;

				// Toll camera style: absoluteCoordinates sub-object
			const TSharedPtr<FJsonObject> *AbsCoords = nullptr;
			if ((*PropsObj)->TryGetObjectField(TEXT("absoluteCoordinates"), AbsCoords) && AbsCoords)
			{
				(*AbsCoords)->TryGetNumberField(TEXT("latitude"),  Location.X);
				(*AbsCoords)->TryGetNumberField(TEXT("longitude"), Location.Y);
			}

			// TRACI style: direct latitude/longitude at properties root
			if (Location.X == 0.0 && Location.Y == 0.0)
			{
				(*PropsObj)->TryGetNumberField(TEXT("latitude"),  Location.X);
				(*PropsObj)->TryGetNumberField(TEXT("longitude"), Location.Y);
			}

			if (Location.X != 0.0 || Location.Y != 0.0)
				break;
			}
		}
	}

	const double PrevLat = LastLatitude;
	const double PrevLon = LastLongitude;
	LastLatitude  = Location.X;
	LastLongitude = Location.Y;

	const bool bMoved = FMath::Abs(LastLatitude - PrevLat) > 1e-7 || FMath::Abs(LastLongitude - PrevLon) > 1e-7;

	if (bSnappedToGround && !bMoved)
	{
		// Attribute update with no position change — keep the snapped location.
		return;
	}

	SetMovementTarget(Location.X, Location.Y, 0.0);
}

// ============================================================
//  SetMovementTarget — called by patch handlers to set destination + speed
// ============================================================

void ATempUIActor::SetMovementTarget(double Lat, double Lon, double SpeedKmh, bool bTeleport)
{
	// SpeedKmh is accepted for API compatibility with callers that report it (TRACI
	// payloads), but interpolation is now paced by the actual observed update cadence
	// (EstimatedUpdateInterval) instead — see Tick() for why.

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (!bHasInterpolationTarget || bTeleport)
	{
		// Teleport the visual position — used on first-ever placement and on the
		// first live WebSocket update so stale REST data doesn't create a ghost trail.
		VisualLatitude  = Lat;
		VisualLongitude = Lon;
		bHasInterpolationTarget = true;
		{
			const double UseHeight = bHasExplicitAltitude ? LastExplicitAltitude : LastSnappedAltitudeMeters;
			VisualAltitudeMeters = UseHeight;
			GlobeAnchor->MoveToLongitudeLatitudeHeight(FVector(Lon, Lat, UseHeight));
		}
		// Start == target, so Tick()'s lerp is a no-op until the next real leg begins.
		InterpStartLat    = Lat;
		InterpStartLon    = Lon;
		InterpStartHeight = VisualAltitudeMeters;
		InterpStartPitch  = VisualPitchDeg;
		InterpStartTime   = Now;
	}
	else
	{
		if (LastTargetSetTime > 0.0)
		{
			// Blend toward a running average so brief bursts don't skew the estimate.
			const double Measured = Now - LastTargetSetTime;
			if (Measured > 0.05 && Measured < 10.0)
				EstimatedUpdateInterval = FMath::Lerp(EstimatedUpdateInterval, Measured, 0.25);
		}

		// Snapshot wherever the car actually is right now (mid-lerp) as the start of a
		// fresh leg toward the new target — see Tick() for why this makes motion a true
		// constant-rate lerp instead of a filter that can freeze or snap.
		InterpStartLat    = VisualLatitude;
		InterpStartLon    = VisualLongitude;
		InterpStartHeight = VisualAltitudeMeters;
		InterpStartPitch  = VisualPitchDeg;
		InterpStartTime   = Now;
	}
	LastTargetSetTime = Now;

	LastLatitude  = Lat;
	LastLongitude = Lon;
	TriggerSnapIfNeeded();
}

void ATempUIActor::SetMovementTarget(double Lat, double Lon, double SpeedKmh, double AngleDeg, double AccelMs2, bool bTeleport)
{
	bHasExplicitAngle = true;
	LastAngleDeg = AngleDeg;
	SetMovementTarget(Lat, Lon, SpeedKmh, bTeleport);
}

// ============================================================
//  Tick — smoothly interpolate visual position toward server target
// ============================================================

void ATempUIActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bHasInterpolationTarget)
		return;

	// True constant-rate linear interpolation: Alpha is recomputed fresh each frame from
	// real elapsed time since the leg started, over the actual observed update cadence
	// (EstimatedUpdateInterval) — not the payload's reported speed. That guarantees the
	// car always arrives exactly when the next update is expected: no early-arrival
	// freeze (reported speed too fast for the real distance) and no late-arrival snap
	// (reported speed too slow) — the "fwoop...stop...fwoop" stutter was exactly that
	// mismatch between assumed and real per-update travel time.
	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	const double Duration = FMath::Max(EstimatedUpdateInterval, 0.1);
	const double Alpha = FMath::Clamp((Now - InterpStartTime) / Duration, 0.0, 1.0);

	VisualLatitude  = FMath::Lerp(InterpStartLat, LastLatitude, Alpha);
	VisualLongitude = FMath::Lerp(InterpStartLon, LastLongitude, Alpha);

	// Height/pitch share the same Alpha, so they arrive at their targets in lockstep
	// with lat/lon — valid because SnapToGround now traces at the same destination
	// (LastLatitude/LastLongitude) lat/lon is heading toward.
	const double TargetHeight = bHasExplicitAltitude ? LastExplicitAltitude : LastSnappedAltitudeMeters;
	VisualAltitudeMeters = FMath::Lerp(InterpStartHeight, TargetHeight, Alpha);
	VisualPitchDeg       = FMath::Lerp(InterpStartPitch, LastSnappedPitchDeg, Alpha);

	GlobeAnchor->MoveToLongitudeLatitudeHeight(FVector(VisualLongitude, VisualLatitude, VisualAltitudeMeters));

	// Rotate to face heading. Prefer the server-reported angle when available; fall back
	// to deriving direction from the whole leg (start -> target, not frame-to-frame,
	// which would be noisy right as Alpha approaches 1 and per-frame motion shrinks).
	ACesiumGeoreference *GeoRef = ACesiumGeoreference::GetDefaultGeoreference(GetWorld());
	if (GeoRef)
	{
		if (bHasExplicitAngle)
		{
			// TRACI angle: 0=North, 90=East, clockwise. Convert to ESU yaw (0=East).
			const double EsuYawDeg = LastAngleDeg - 90.0;
			const FRotator WorldRot = GeoRef->TransformEastSouthUpRotatorToUnreal(
				FRotator(VisualPitchDeg, EsuYawDeg, 0.0), GetActorLocation());
			SetActorRotation(WorldRot);
		}
		else
		{
			const double LegDLatM = (LastLatitude - InterpStartLat) * 111000.0;
			const double LegDLonM = (LastLongitude - InterpStartLon) * 111000.0 * FMath::Cos(FMath::DegreesToRadians(InterpStartLat));
			if (FMath::Sqrt(LegDLatM * LegDLatM + LegDLonM * LegDLonM) > 0.5)
			{
				// In Cesium's ESU frame: X=East, Y=South, Z=Up.
				// atan2(-dLatM, dLonM) gives the ESU yaw for a movement of (dEast, dNorth).
				const double EsuYawDeg = FMath::RadiansToDegrees(FMath::Atan2(-LegDLatM, LegDLonM));
				const FRotator WorldRot = GeoRef->TransformEastSouthUpRotatorToUnreal(
					FRotator(VisualPitchDeg, EsuYawDeg, 0.0), GetActorLocation());
				SetActorRotation(WorldRot);
			}
		}
	}
}

// ============================================================
//  TriggerSnapIfNeeded — re-trace ground at the new target on every live update
// ============================================================

void ATempUIActor::TriggerSnapIfNeeded()
{
	if (bHasExplicitAltitude)
		return;

	if (bSnapInProgress)
		return;

	if (!bSnappedToGround)
	{
		// Not yet snapped for the first time — the periodic CheckVisibility timer
		// (started in BeginPlay) owns the cold-start snap once this entity is actually
		// on screen. Nothing to do here until that first snap succeeds.
		return;
	}

	// Already snapped once: trace fresh ground/pitch at the NEW server target position
	// (LastLatitude/LastLongitude) — not wherever the car currently is — every time a
	// live update arrives. That makes LastSnappedAltitudeMeters/LastSnappedPitchDeg
	// genuine interpolation targets Tick() can approach in lockstep with the lat/lon
	// move (same destination, same progress fraction), instead of a stale value that
	// needs separate dead-reckoning/smoothing between periodic re-traces.
	if (!IsInCameraView() || !IsWithinSnapRange(LastLatitude, LastLongitude))
		return;

	SnapToGround(LastLatitude, LastLongitude);
}

// ============================================================
//  IsInCameraView / IsWithinSnapRange / CheckVisibility
// ============================================================

bool ATempUIActor::IsInCameraView() const
{
	UWorld *World = GetWorld();
	APlayerController *PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
		return false;

	FVector2D ScreenPos;
	if (!UGameplayStatics::ProjectWorldToScreen(PC, GetActorLocation(), ScreenPos))
		return false;

	int32 SX, SY;
	PC->GetViewportSize(SX, SY);
	return ScreenPos.X >= 0 && ScreenPos.X <= SX &&
		   ScreenPos.Y >= 0 && ScreenPos.Y <= SY;
}

bool ATempUIActor::IsWithinSnapRange(double Lat, double Lon) const
{
	UWorld *World = GetWorld();
	APlayerController *PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
		return false;

	ACesiumGeoreference *GeoRef = ACesiumGeoreference::GetDefaultGeoreference(World);
	if (!GeoRef)
		return false;

	FVector CamUEPos;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamUEPos, CamRot);

	FVector CamLLH = GeoRef->TransformUnrealPositionToLongitudeLatitudeHeight(CamUEPos);
	// CamLLH.X = Longitude, CamLLH.Y = Latitude

	const double dLat = (Lat - CamLLH.Y) * 111000.0;
	const double dLon = (Lon - CamLLH.X) * 111000.0 * FMath::Cos(FMath::DegreesToRadians(Lat));
	constexpr double MaxDistMetres = 5000.0;
	return FMath::Sqrt(dLat * dLat + dLon * dLon) <= MaxDistMetres;
}

void ATempUIActor::CheckVisibility()
{
	if (!IsInCameraView())
		return;

	if (!IsWithinSnapRange(VisualLatitude, VisualLongitude))
		return;

	UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: IN VIEW"), *ThingId);

	SnapToGround(VisualLatitude, VisualLongitude);
}

// ============================================================
//  SnapToGround — deferred terrain-surface placement
// ============================================================

void ATempUIActor::SnapToGround(double TraceLatitude, double TraceLongitude)
{
	if (bHasExplicitAltitude)
		return;

	UWorld *World = GetWorld();
	if (!World || (TraceLatitude == 0.0 && TraceLongitude == 0.0))
		return;

	// Classify tilesets so we can verify the trace hit the P3D mesh, not CWT.
	ACesium3DTileset *P3DTileset = nullptr;
	ACesium3DTileset *TerrainTileset = nullptr;
	for (TActorIterator<ACesium3DTileset> It(World); It; ++It)
	{
		ACesium3DTileset *Candidate = *It;
		const FString Label = Candidate->GetActorNameOrLabel();
		if (Label.Contains(TEXT("Terrain"), ESearchCase::IgnoreCase))
			TerrainTileset = Candidate;
		else if (!P3DTileset)
			P3DTileset = Candidate;
	}

	// Keep only upward-facing surfaces (normal Z > 0.1, approximately "up" near the
	// georeference origin) to skip mesh undersides and vertical walls.
	// Among valid hits pick the HIGHEST Z — the first surface encountered coming down
	// from above — so subsurface photogrammetry artifacts never win over the real ground.
	auto PickHighestUpwardHit = [](const TArray<FHitResult> &Hits) -> const FHitResult *
	{
		const FHitResult *Best = nullptr;
		for (const FHitResult &Hit : Hits)
		{
			if (Hit.ImpactNormal.Z < 0.1f)
				continue;
			if (!Best || Hit.ImpactPoint.Z > Best->ImpactPoint.Z)
				Best = &Hit;
		}
		return Best;
	};

	// For the front/rear pitch traces, "highest" is the wrong heuristic: a guardrail,
	// curb, or wall cap just off the road is often higher than the actual road surface
	// a metre or two ahead/behind, and "highest wins" would happily grab its top instead
	// of the road — producing wildly wrong pitches (we saw e.g. -31.6deg logged for a
	// stationary road sign that should read ~0deg). The road surface a car-length away
	// should be close in height to the surface right under the car, so pick whichever
	// upward-facing hit is CLOSEST to the center trace's height instead.
	auto PickClosestUpwardHit = [](const TArray<FHitResult> &Hits, double ReferenceZ) -> const FHitResult *
	{
		const FHitResult *Best = nullptr;
		double BestDist = 0.0;
		for (const FHitResult &Hit : Hits)
		{
			if (Hit.ImpactNormal.Z < 0.1f)
				continue;
			const double Dist = FMath::Abs(Hit.ImpactPoint.Z - ReferenceZ);
			if (!Best || Dist < BestDist)
			{
				Best = &Hit;
				BestDist = Dist;
			}
		}
		return Best;
	};

	// Line trace — hits any geometry except CWT and self.
	// Accepts P3D tiles (if collision enabled), testtalude, or any static mesh.
	{
		FVector TraceStart = UCoordinatesConversionService::Get()->ConvertWSG84ToUELocal(TraceLatitude, TraceLongitude, 5000.0);
		FVector TraceEnd   = UCoordinatesConversionService::Get()->ConvertWSG84ToUELocal(TraceLatitude, TraceLongitude, -500.0);

		TArray<FHitResult> Hits;
		FCollisionQueryParams Params(NAME_None, /*bTraceComplex=*/true);
		Params.AddIgnoredActor(this);

		// ObjectType query returns ALL hits without stopping at the first blocker,
		// so we get both CWT (above) and P3D (below it in road-cut sections).
		FCollisionObjectQueryParams ObjectParams(ECC_WorldStatic);
		World->LineTraceMultiByObjectType(Hits, TraceStart, TraceEnd, ObjectParams, Params);

		// Default to the HIGHEST upward-facing hit — stable and self-consistent (doesn't
		// depend on any prior state), needed to skip past the CWT terrain mesh sitting
		// below the real P3D road surface in cut sections. Only override with "closest to
		// last known height" when there's a hit near our established height that is
		// SIGNIFICANTLY lower than the highest one (>1.5m) — i.e. a genuine stacked-deck
		// situation (overpass/bridge), not just two hits a few cm apart from mesh/tile
		// seam noise. Without that gate, "closest to last" is a feedback loop: whichever
		// hit gets picked becomes next frame's reference, so two nearly-identical nearby
		// hits can flip-flop pick to pick and visibly bounce the car up and down.
		const FHitResult *BestHit = PickHighestUpwardHit(Hits);
		if (bSnappedToGround && BestHit && FMath::Abs(BestHit->ImpactPoint.Z - LastSnappedRawZ) > 150.0)
		{
			if (const FHitResult *Closest = PickClosestUpwardHit(Hits, LastSnappedRawZ))
				BestHit = Closest;
		}

		UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: trace %d hits, best: %s"),
			   *ThingId, Hits.Num(), BestHit ? *GetNameSafe(BestHit->GetActor()) : TEXT("none"));

		if (BestHit)
		{
			// Only record the traced target height/pitch here — never move the actor
			// directly. Tick() owns all actual position/height/pitch application so a
			// fresh trace result eases in smoothly instead of teleporting the actor to
			// the target position underneath the in-flight horizontal interpolation.
			ACesiumGeoreference *GeoRef = ACesiumGeoreference::GetDefaultGeoreference(World);
			if (!GeoRef)
				return;

			FVector LLH = GeoRef->TransformUnrealPositionToLongitudeLatitudeHeight(BestHit->Location);
			// The default cube mesh is centered on the actor's pivot, so placing the pivot
			// exactly at the traced ground point buries the mesh's bottom half by design —
			// offset up by half the box height so the vehicle's underside rests on the
			// surface instead. Only valid once ApplyScale has actually sized InteractionBox
			// to match that mesh (see bHasAppliedScale); entities with a Blueprint-assigned,
			// typically bottom-pivoted mesh (signs, poles, cameras) get no offset at all.
			const double BoxHalfHeightM = bHasAppliedScale ? (InteractionBox->GetScaledBoxExtent().Z / 100.0) : 0.0;
			const double NewAltitude = LLH.Z + BoxHalfHeightM;

			// Convergence check: rather than trusting any single hit as final (even a P3D
			// mesh hit can be a coarse LOD that gets replaced by a higher-detail tile a
			// moment later), keep retracing until two consecutive snaps agree closely on
			// height. Only meaningful once we already have a prior snap to compare against.
			const bool bWasAlreadySnapped = bSnappedToGround;
			const double PrevAltitude = LastSnappedAltitudeMeters;
			const bool bConverged = bWasAlreadySnapped && FMath::Abs(NewAltitude - PrevAltitude) < ConvergenceThresholdMeters;

			LastSnappedRawZ = BestHit->Location.Z;
			LastSnappedAltitudeMeters = NewAltitude;
			bSnappedToGround = true;

			// Front/rear traces (offset along the vehicle's current forward axis) give the
			// incline pitch — reuses the same trace already paid for above, just offset.
			// Left at its last known value if either side misses (e.g. edge of loaded tiles),
			// so a momentary trace gap doesn't pop the car back to flat.
			const double HalfLengthUU = InteractionBox->GetScaledBoxExtent().X;
			const FVector ForwardOffset = GetActorForwardVector() * HalfLengthUU;

			TArray<FHitResult> FrontHits, RearHits;
			World->LineTraceMultiByObjectType(FrontHits, TraceStart + ForwardOffset, TraceEnd + ForwardOffset, ObjectParams, Params);
			World->LineTraceMultiByObjectType(RearHits,  TraceStart - ForwardOffset, TraceEnd - ForwardOffset, ObjectParams, Params);
			const FHitResult *FrontHit = PickClosestUpwardHit(FrontHits, BestHit->ImpactPoint.Z);
			const FHitResult *RearHit  = PickClosestUpwardHit(RearHits, BestHit->ImpactPoint.Z);
			if (FrontHit && RearHit)
			{
				// Positive pitch = nose up (UE convention). Front higher than rear means the
				// road rises ahead, so the nose should tilt up to lie flat on the slope.
				// Clamped as a safety net — real roads here don't exceed ~25 deg grade, so
				// anything beyond that is still a bad hit (e.g. a nearby wall/railing) rather
				// than a real slope, and shouldn't be allowed to bury the car.
				const double RawPitchDeg = FMath::RadiansToDegrees(
					FMath::Atan2(FrontHit->ImpactPoint.Z - RearHit->ImpactPoint.Z, 2.0 * HalfLengthUU));
				LastSnappedPitchDeg = FMath::Clamp(RawPitchDeg, -25.0, 25.0);
			}

			if (bConverged)
			{
				// Height has stabilised across two consecutive snaps — stop the periodic
				// CheckVisibility timer; TriggerSnapIfNeeded takes over from here, tracing
				// fresh on every live position update instead of polling.
				GetWorldTimerManager().ClearTimer(VisibilityCheckTimer);
				UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: snapped to mesh surface '%s', pitch=%.1fdeg (converged)"),
					   *ThingId, *GetNameSafe(BestHit->GetActor()), LastSnappedPitchDeg);
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: snapped to mesh surface '%s', pitch=%.1fdeg (still converging, delta=%.2fm)"),
					   *ThingId, *GetNameSafe(BestHit->GetActor()), LastSnappedPitchDeg, FMath::Abs(NewAltitude - PrevAltitude));
			}
			return;
		}
	}

	// P3D tiles not yet loaded or no collision — use CWT async as a temporary position.
	// CheckVisibility will retry the trace next second.
	if (!TerrainTileset)
		return;

	// Batch center + front + rear samples into the one async action so incline pitch
	// (see OnGroundHeightSampled) doesn't cost a second round-trip.
	TArray<FVector> Positions = {FVector(TraceLongitude, TraceLatitude, 0.0)};
	ACesiumGeoreference *GeoRefForPitch = ACesiumGeoreference::GetDefaultGeoreference(World);
	if (GeoRefForPitch)
	{
		// Offset from the TRACE position (not GetActorLocation()) — the trace target can
		// be far from wherever the actor is currently rendered when tracing at the new
		// server destination rather than the car's current position.
		const double HalfLengthUU = InteractionBox->GetScaledBoxExtent().X;
		const FVector ForwardOffset = GetActorForwardVector() * HalfLengthUU;
		const FVector TraceBaseLoc = UCoordinatesConversionService::Get()->ConvertWSG84ToUELocal(TraceLatitude, TraceLongitude, 0.0);
		const FVector FrontLLH = GeoRefForPitch->TransformUnrealPositionToLongitudeLatitudeHeight(TraceBaseLoc + ForwardOffset);
		const FVector RearLLH  = GeoRefForPitch->TransformUnrealPositionToLongitudeLatitudeHeight(TraceBaseLoc - ForwardOffset);
		Positions.Add(FVector(FrontLLH.X, FrontLLH.Y, 0.0));
		Positions.Add(FVector(RearLLH.X, RearLLH.Y, 0.0));
	}

	UCesiumSampleHeightMostDetailedAsyncAction *Action =
		UCesiumSampleHeightMostDetailedAsyncAction::SampleHeightMostDetailed(TerrainTileset, Positions);
	if (!Action)
		return;

	bSnapInProgress = true;
	Action->OnHeightsSampled.AddDynamic(this, &ATempUIActor::OnGroundHeightSampled);
	Action->Activate();
}

void ATempUIActor::OnGroundHeightSampled(const TArray<FCesiumSampleHeightResult> &Results, const TArray<FString> &Warnings)
{
	if (Results.IsEmpty())
	{
		bSnapInProgress = false;
		UE_LOG(LogTemp, Warning, TEXT("TempUIActor [%s]: height sample returned no results"), *ThingId);
		return;
	}

	const FCesiumSampleHeightResult &Result = Results[0];
	if (!Result.SampleSuccess)
	{
		bSnapInProgress = false;
		UE_LOG(LogTemp, Warning, TEXT("TempUIActor [%s]: height sample failed (no tile coverage at this position)"), *ThingId);
		return;
	}

	// Only record the sampled target height/pitch — Tick() applies them directly next
	// frame (see the line-trace path above for why no smoothing is needed/wanted here).
	bSnapInProgress = false;
	// Offset up by half the box height so the car's underside rests on the sampled
	// surface instead of its center-pivoted mesh burying its bottom half — matches the
	// line-trace path. Only valid once ApplyScale has actually sized InteractionBox to
	// match the default cube mesh (see bHasAppliedScale) — see line-trace path for why.
	const double BoxHalfHeightM = bHasAppliedScale ? (InteractionBox->GetScaledBoxExtent().Z / 100.0) : 0.0;
	const double NewAltitude = Result.LongitudeLatitudeHeight.Z + BoxHalfHeightM;

	// Same convergence check as the line-trace path (see there) — a CWT sample can also be
	// a coarse/interim result, so don't trust it as final until two consecutive snaps agree.
	const bool bWasAlreadySnapped = bSnappedToGround;
	const double PrevAltitude = LastSnappedAltitudeMeters;
	const bool bConverged = bWasAlreadySnapped && FMath::Abs(NewAltitude - PrevAltitude) < ConvergenceThresholdMeters;

	bSnappedToGround = true;
	LastSnappedAltitudeMeters = NewAltitude;
	// Keep LastSnappedRawZ consistent so a line-trace right after a CWT fallback still
	// has a meaningful reference for picking among stacked hits (see SnapToGround()).
	LastSnappedRawZ = UCoordinatesConversionService::Get()->ConvertWSG84ToUELocal(
		Result.LongitudeLatitudeHeight.Y, Result.LongitudeLatitudeHeight.X, Result.LongitudeLatitudeHeight.Z).Z;

	// Front/rear samples (if requested) — see SnapToGround's Positions batching above.
	if (Results.Num() >= 3 && Results[1].SampleSuccess && Results[2].SampleSuccess)
	{
		const double HalfLengthM = InteractionBox->GetScaledBoxExtent().X / 100.0;
		// Positive pitch = nose up (UE convention), matching the line-trace path. Clamped
		// the same way as the line-trace path — see there for why.
		const double RawPitchDeg = FMath::RadiansToDegrees(FMath::Atan2(
			Results[1].LongitudeLatitudeHeight.Z - Results[2].LongitudeLatitudeHeight.Z, 2.0 * HalfLengthM));
		LastSnappedPitchDeg = FMath::Clamp(RawPitchDeg, -25.0, 25.0);
	}

	if (bConverged)
	{
		// Height has stabilised across two consecutive snaps — stop the periodic
		// CheckVisibility timer; TriggerSnapIfNeeded takes over from here, tracing fresh on
		// every live position update instead of polling.
		GetWorldTimerManager().ClearTimer(VisibilityCheckTimer);
		UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: snapped to CWT ground — sampled height=%.1fm (converged)"),
			   *ThingId, Result.LongitudeLatitudeHeight.Z);
	}
	else
	{
		// Leave VisibilityCheckTimer running — a stationary entity (sign, pole) never moves
		// and so never re-triggers a trace via TriggerSnapIfNeeded; this periodic retry is
		// its only path to correct an initial coarse/interim placement instead of floating
		// or sinking permanently once tiles finish streaming in at higher detail.
		UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: snapped to CWT ground — sampled height=%.1fm (still converging, delta=%.2fm)"),
			   *ThingId, Result.LongitudeLatitudeHeight.Z, FMath::Abs(NewAltitude - PrevAltitude));
	}
}

// ============================================================
//  GLB model loading
// ============================================================

FString ATempUIActor::GetDefaultPolygonUrl() const
{
	if (!StructInstance.IsValid() || !DataStructType)
		return FString();

	for (TFieldIterator<FProperty> It(DataStructType); It; ++It)
	{
		if (!It->GetName().Equals(TEXT("attributes"), ESearchCase::IgnoreCase))
			continue;

		FStructProperty *StructProp = CastField<FStructProperty>(*It);
		if (!StructProp)
			continue;

		const void *AttribPtr = StructProp->ContainerPtrToValuePtr<void>(StructInstance->GetStructMemory());

		for (TFieldIterator<FProperty> AttrIt(StructProp->Struct); AttrIt; ++AttrIt)
		{
			if (!AttrIt->GetName().Equals(TEXT("polygon"), ESearchCase::IgnoreCase))
				continue;

			FStrProperty *StrProp = CastField<FStrProperty>(*AttrIt);
			if (!StrProp)
				continue;

			return StrProp->GetPropertyValue_InContainer(AttribPtr);
		}
	}

	return FString();
}

void ATempUIActor::TryLoadGlbModel()
{
	if (!RawJson.IsValid())
		return;

	// Fire entities carry an array of polygon URLs; use the dedicated multi-model path.
	if (DataStructType == FIgnitionPointData::StaticStruct())
	{
		TryLoadFireGlbModels();
		return;
	}

	FString PolygonUrl;

	const TSharedPtr<FJsonObject> *AttribObj = nullptr;
	if (RawJson->TryGetObjectField(TEXT("attributes"), AttribObj) && AttribObj)
		(*AttribObj)->TryGetStringField(TEXT("polygon"), PolygonUrl);

	// Ditto didn't send a polygon URL — fall back to the struct's compiled-in default
	// (e.g. the S3 URL baked into FInfPtIluminacaoAttributes/FInfPtSinalizacaoAttributes).
	if (PolygonUrl.IsEmpty())
		PolygonUrl = GetDefaultPolygonUrl();

	if (PolygonUrl.IsEmpty())
		return;

	if (PolygonUrl == LoadedPolygonUrl)
		return;

	LoadedPolygonUrl = PolygonUrl;

	UGameInstance *GI = GetGameInstance();
	if (!GI)
		return;

	UGlbModelService *Svc = GI->GetSubsystem<UGlbModelService>();
	if (!Svc)
		return;

	FOnGlbMeshLayersLoaded Callback;
	Callback.BindDynamic(this, &ATempUIActor::OnPolygonGlbLayersLoaded);
	Svc->RequestMeshLayers(PolygonUrl, Callback);
}

// ─── Fire: multi-model GLB loading ───────────────────────────────────────────

void ATempUIActor::TryLoadFireGlbModels()
{
	if (!RawJson.IsValid())
		return;

	const TSharedPtr<FJsonObject> *AttribObj = nullptr;
	if (!RawJson->TryGetObjectField(TEXT("attributes"), AttribObj) || !AttribObj)
		return;

	const TArray<TSharedPtr<FJsonValue>> *PolygonArr = nullptr;
	if (!(*AttribObj)->TryGetArrayField(TEXT("polygon"), PolygonArr) || !PolygonArr || PolygonArr->IsEmpty())
		return;

	UGameInstance *GI = GetGameInstance();
	if (!GI)
		return;
	UGlbModelService *Svc = GI->GetSubsystem<UGlbModelService>();
	if (!Svc)
		return;

	// polygon[0] = fire cone GLB  →  layer "Cone"  (visible by default)
	// polygon[1] = simulation GLB →  layer "Simulation" (shown once step GeoJSONs load)
	for (int32 i = 0; i < PolygonArr->Num(); i++)
	{
		const FString Url = (*PolygonArr)[i]->AsString();
		if (Url.IsEmpty() || FireGlbLoadedUrls.Contains(Url))
			continue;

		FireGlbLoadedUrls.Add(Url);

		FOnGlbMeshLayersLoaded Callback;
		if (i == 0)
			Callback.BindDynamic(this, &ATempUIActor::OnConeGlbLayersLoaded);
		else
			Callback.BindDynamic(this, &ATempUIActor::OnSimulationGlbLayersLoaded);

		Svc->RequestMeshLayers(Url, Callback);
	}

	TryFetchFirePerimeters();
}

void ATempUIActor::OnConeGlbLayersLoaded(const TArray<FGlbMeshLayer>& GlbLayers)
{
	if (GlbLayers.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("TempUIActor [%s]: cone GLB failed to load"), *ThingId);
		return;
	}
	AddOrReplaceMeshLayerGroup(TEXT("Cone"), GlbLayers);
	StaticMeshComponent->SetVisibility(false);
	UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: cone GLB loaded (%d mesh layer(s))"), *ThingId, GlbLayers.Num());
	SpawnTerrainExclusionPolygon();
}

void ATempUIActor::OnSimulationGlbLayersLoaded(const TArray<FGlbMeshLayer>& GlbLayers)
{
	if (GlbLayers.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("TempUIActor [%s]: simulation GLB failed to load"), *ThingId);
		return;
	}
	AddOrReplaceMeshLayerGroup(TEXT("Simulation"), GlbLayers);
	if (bSimulationStepsReady)
	{
		// Steps already completed before the GLB arrived — switch now.
		SetLayerGroupVisible(TEXT("Simulation"), true);
		SetLayerGroupVisible(TEXT("Cone"), false);
		UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: simulation GLB loaded (%d mesh layer(s), steps already ready, switching)"), *ThingId, GlbLayers.Num());
	}
	else
	{
		SetLayerGroupVisible(TEXT("Simulation"), false);
		UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: simulation GLB loaded (%d mesh layer(s), hidden until steps ready)"), *ThingId, GlbLayers.Num());
	}

	// SetMeshLayerVisible() above only re-traces an existing polygon; make sure one exists
	// even if the cone GLB never loaded (e.g. it failed or this entity has no cone model).
	if (!TerrainExclusionPolygon)
		SpawnTerrainExclusionPolygon();
}

// ─── Fire: GeoJSON perimeter fetching ────────────────────────────────────────

// Andrew's monotone chain: returns the convex hull of Points in counter-clockwise order.
static TArray<FVector2D> ComputeConvexHull2D(TArray<FVector2D> Points)
{
	Points.Sort([](const FVector2D& A, const FVector2D& B)
	{
		return A.X != B.X ? A.X < B.X : A.Y < B.Y;
	});

	const int32 N = Points.Num();
	if (N < 3)
		return Points;

	auto Cross = [](const FVector2D& O, const FVector2D& A, const FVector2D& B)
	{
		return (A.X - O.X) * (B.Y - O.Y) - (A.Y - O.Y) * (B.X - O.X);
	};

	TArray<FVector2D> Hull;
	Hull.SetNum(2 * N);
	int32 K = 0;

	// Lower hull.
	for (int32 i = 0; i < N; i++)
	{
		while (K >= 2 && Cross(Hull[K - 2], Hull[K - 1], Points[i]) <= 0)
			K--;
		Hull[K++] = Points[i];
	}

	// Upper hull.
	for (int32 i = N - 2, LowerCount = K + 1; i >= 0; i--)
	{
		while (K >= LowerCount && Cross(Hull[K - 2], Hull[K - 1], Points[i]) <= 0)
			K--;
		Hull[K++] = Points[i];
	}

	Hull.SetNum(K - 1); // last point duplicates the first
	return Hull;
}

// Resolves a GeoJSON "geometry" object's outer ring (Polygon: coordinates[0]; MultiPolygon:
// coordinates[0][0] — outer ring of the first polygon) into world lat/lon points.
static TArray<FVector2D> ExtractOuterRingFromGeometry(const TSharedPtr<FJsonObject>& Geom)
{
	TArray<FVector2D> Out;
	if (!Geom.IsValid())
		return Out;

	FString GeomType;
	Geom->TryGetStringField(TEXT("type"), GeomType);

	const TArray<TSharedPtr<FJsonValue>> *Coords = nullptr;
	if (!Geom->TryGetArrayField(TEXT("coordinates"), Coords) || !Coords || Coords->IsEmpty())
		return Out;

	const TArray<TSharedPtr<FJsonValue>> *OuterRing = nullptr;
	if (GeomType == TEXT("MultiPolygon"))
	{
		if ((*Coords)[0]->Type == EJson::Array)
		{
			const TArray<TSharedPtr<FJsonValue>> &FirstPoly = (*Coords)[0]->AsArray();
			if (!FirstPoly.IsEmpty() && FirstPoly[0]->Type == EJson::Array)
				OuterRing = &FirstPoly[0]->AsArray();
		}
	}
	else // Polygon
	{
		if ((*Coords)[0]->Type == EJson::Array)
			OuterRing = &(*Coords)[0]->AsArray();
	}

	if (!OuterRing)
		return Out;

	for (const TSharedPtr<FJsonValue> &PtVal : *OuterRing)
	{
		if (PtVal->Type != EJson::Array)
			continue;
		const TArray<TSharedPtr<FJsonValue>> &Coord = PtVal->AsArray();
		if (Coord.Num() >= 2)
			Out.Add(FVector2D(Coord[1]->AsNumber(), Coord[0]->AsNumber())); // X=lat, Y=lon
	}

	return Out;
}

TArray<FVector2D> ATempUIActor::ParseGeoJsonOuterRing(const FString &JsonStr)
{
	TSharedPtr<FJsonObject> Root;
	{
		TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(JsonStr);
		if (!FJsonSerializer::Deserialize(R, Root) || !Root.IsValid())
			return {};
	}

	// Unwrap FeatureCollection → Feature → geometry object
	TSharedPtr<FJsonObject> Geom = Root;
	FString Type;
	Root->TryGetStringField(TEXT("type"), Type);

	if (Type == TEXT("FeatureCollection"))
	{
		const TArray<TSharedPtr<FJsonValue>> *Feats = nullptr;
		if (Root->TryGetArrayField(TEXT("features"), Feats) && Feats && !Feats->IsEmpty())
		{
			if (TSharedPtr<FJsonObject> Feat = (*Feats)[0]->AsObject())
			{
				const TSharedPtr<FJsonObject> *GP = nullptr;
				if (Feat->TryGetObjectField(TEXT("geometry"), GP) && GP)
					Geom = *GP;
			}
		}
	}
	else if (Type == TEXT("Feature"))
	{
		const TSharedPtr<FJsonObject> *GP = nullptr;
		if (Root->TryGetObjectField(TEXT("geometry"), GP) && GP)
			Geom = *GP;
	}

	return ExtractOuterRingFromGeometry(Geom);
}

// The cone GeoJSON response is a FeatureCollection with one Polygon feature per time horizon
// (30/60/90/120 min, etc). Consecutive horizon bands share an edge, so the true cone shape is
// the union of every section — reading only features[0] (as ParseGeoJsonOuterRing does) leaves
// just the first, smallest wedge. Combine every section's points and take the convex hull: the
// daisy-chained wedge sections are already close to convex, so this doesn't meaningfully
// over-cover. (Simulation-step perimeters are a single, highly concave burn-scar ring and must
// NOT go through this path — they keep using ParseGeoJsonOuterRing unchanged.)
TArray<FVector2D> ATempUIActor::ParseConeGeoJsonHull(const FString &JsonStr)
{
	TSharedPtr<FJsonObject> Root;
	{
		TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(JsonStr);
		if (!FJsonSerializer::Deserialize(R, Root) || !Root.IsValid())
			return {};
	}

	FString Type;
	Root->TryGetStringField(TEXT("type"), Type);

	TArray<FVector2D> AllPoints;

	if (Type == TEXT("FeatureCollection"))
	{
		const TArray<TSharedPtr<FJsonValue>> *Feats = nullptr;
		if (Root->TryGetArrayField(TEXT("features"), Feats) && Feats)
		{
			for (const TSharedPtr<FJsonValue> &FeatVal : *Feats)
			{
				if (TSharedPtr<FJsonObject> Feat = FeatVal->AsObject())
				{
					const TSharedPtr<FJsonObject> *GP = nullptr;
					if (Feat->TryGetObjectField(TEXT("geometry"), GP) && GP)
						AllPoints.Append(ExtractOuterRingFromGeometry(*GP));
				}
			}
		}
	}
	else if (Type == TEXT("Feature"))
	{
		const TSharedPtr<FJsonObject> *GP = nullptr;
		if (Root->TryGetObjectField(TEXT("geometry"), GP) && GP)
			AllPoints.Append(ExtractOuterRingFromGeometry(*GP));
	}
	else
	{
		AllPoints.Append(ExtractOuterRingFromGeometry(Root));
	}

	return ComputeConvexHull2D(AllPoints);
}

void ATempUIActor::TryFetchFirePerimeters()
{
	if (!StructInstance.IsValid() || DataStructType != FIgnitionPointData::StaticStruct())
		return;

	const FIgnitionPointData *Data =
		reinterpret_cast<const FIgnitionPointData *>(StructInstance->GetStructMemory());

	// ── Cone GeoJSON ──────────────────────────────────────────────────────────
	const FString &ConeUrl = Data->features.cone.properties.perimeters;
	if (!ConeUrl.IsEmpty() && ConeUrl != FetchedConeGeoJsonUrl)
	{
		FetchedConeGeoJsonUrl = ConeUrl;
		TWeakObjectPtr<ATempUIActor> WeakThis(this);
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
		Req->SetURL(ConeUrl);
		Req->SetVerb(TEXT("GET"));
		Req->OnProcessRequestComplete().BindLambda(
			[WeakThis](FHttpRequestPtr, FHttpResponsePtr Response, bool bOk)
			{
				if (!WeakThis.IsValid() || !bOk || !Response.IsValid() || Response->GetResponseCode() != 200)
					return;
				ATempUIActor *Self = WeakThis.Get();
				Self->ParsedConePerimeterPoints = ParseConeGeoJsonHull(Response->GetContentAsString());
				UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: cone perimeter parsed (%d pts)"),
					*Self->ThingId, Self->ParsedConePerimeterPoints.Num());
				if (Self->TerrainExclusionPolygon)
				{
					Self->RemoveTerrainExclusionPolygon();
					Self->SpawnTerrainExclusionPolygon();
				}
			});
		Req->ProcessRequest();
	}

	// ── Perimeter step GeoJSONs ───────────────────────────────────────────────
	const TArray<FString> &StepUrls = Data->features.perimeters.properties.perimeters;
	if (StepUrls.IsEmpty())
		return;

	TArray<int32> NewIndices;
	for (int32 i = 0; i < StepUrls.Num(); i++)
		if (!FetchedPerimeterStepUrls.Contains(StepUrls[i]))
			NewIndices.Add(i);

	if (NewIndices.IsEmpty())
		return;

	ParsedPerimeterSteps.SetNum(StepUrls.Num());

	TSharedPtr<int32> Remaining = MakeShared<int32>(NewIndices.Num());
	TWeakObjectPtr<ATempUIActor> WeakThis(this);

	for (int32 Idx : NewIndices)
	{
		const FString &Url = StepUrls[Idx];
		FetchedPerimeterStepUrls.Add(Url);

		// Parse step_NNNN → minutes from filename (e.g. "step_0015" → 15)
		const FString Base = FPaths::GetBaseFilename(Url);
		const int32 StepMin = Base.Len() > 5 ? FCString::Atoi(*Base.Mid(5)) : 0;

		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
		Req->SetURL(Url);
		Req->SetVerb(TEXT("GET"));
		Req->OnProcessRequestComplete().BindLambda(
			[WeakThis, Idx, StepMin, Remaining](FHttpRequestPtr, FHttpResponsePtr Response, bool bOk)
			{
				if (!WeakThis.IsValid())
					return;
				ATempUIActor *Self = WeakThis.Get();

				if (bOk && Response.IsValid() && Response->GetResponseCode() == 200)
				{
					TArray<FVector2D> Points = ParseGeoJsonOuterRing(Response->GetContentAsString());
					if (Self->ParsedPerimeterSteps.IsValidIndex(Idx))
						Self->ParsedPerimeterSteps[Idx] = MoveTemp(Points);
					UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: step %d min parsed (%d pts)"),
						*Self->ThingId, StepMin, Self->ParsedPerimeterSteps[Idx].Num());
				}

				if (--(*Remaining) == 0)
				{
					Self->bSimulationStepsReady = true;
					// Only switch visibility if the simulation GLB is already loaded.
					// SetLayerGroupVisible() below re-traces the exclusion polygon for us.
					if (Self->HasLayerGroup(TEXT("Simulation")))
					{
						Self->SetLayerGroupVisible(TEXT("Simulation"), true);
						Self->SetLayerGroupVisible(TEXT("Cone"), false);
						UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: switched to simulation model"), *Self->ThingId);
					}
					else
					{
						UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: steps ready, waiting for simulation GLB"), *Self->ThingId);
					}
				}
			});
		Req->ProcessRequest();
	}
}

void ATempUIActor::OnPolygonGlbLayersLoaded(const TArray<FGlbMeshLayer>& GlbLayers)
{
	if (GlbLayers.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("TempUIActor [%s]: GLB mesh load failed"), *ThingId);
		return;
	}

	AddOrReplaceMeshLayerGroup(TEXT("Polygon"), GlbLayers);
	StaticMeshComponent->SetVisibility(false);
	UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: GLB mesh applied as Polygon layer group (%d layer(s))"), *ThingId, GlbLayers.Num());

	SpawnTerrainExclusionPolygon();
}

UStaticMeshComponent* ATempUIActor::AddOrReplaceMeshLayer(const FString& LayerName, UStaticMesh* Mesh)
{
	return AddOrReplaceMeshLayerAt(LayerName, Mesh, FTransform::Identity);
}

UStaticMeshComponent* ATempUIActor::AddOrReplaceMeshLayerAt(const FString& LayerName, UStaticMesh* Mesh, const FTransform& RelativeTransform)
{
	if (UStaticMeshComponent** Existing = MeshLayers.Find(LayerName))
	{
		(*Existing)->DestroyComponent();
		MeshLayers.Remove(LayerName);
		OriginalLayerMaterials.Remove(LayerName);
	}

	UStaticMeshComponent* NewComp = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), *LayerName));
	NewComp->SetupAttachment(SceneRoot);
	NewComp->SetStaticMesh(Mesh);
	NewComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NewComp->SetRelativeTransform(RelativeTransform);
	NewComp->RegisterComponent();

	MeshLayers.Add(LayerName, NewComp);
	OnMeshLayersChanged.Broadcast();

	return NewComp;
}

void ATempUIActor::AddOrReplaceMeshLayerGroup(const FString& GroupName, const TArray<FGlbMeshLayer>& GlbLayers)
{
	// Remove every layer this group previously created before adding the new set —
	// a reload may return a different node count/names than last time.
	if (TArray<FString>* PrevNames = MeshLayerGroups.Find(GroupName))
	{
		for (const FString& PrevName : *PrevNames)
		{
			if (UStaticMeshComponent** Existing = MeshLayers.Find(PrevName))
			{
				(*Existing)->DestroyComponent();
				MeshLayers.Remove(PrevName);
				OriginalLayerMaterials.Remove(PrevName);
			}
		}
	}

	TArray<FString>& NewNames = MeshLayerGroups.Add(GroupName);
	NewNames.Reserve(GlbLayers.Num());

	const bool bSingleLayer = GlbLayers.Num() == 1;
	TSet<FString> UsedNames;

	for (const FGlbMeshLayer& Layer : GlbLayers)
	{
		if (!Layer.Mesh)
			continue;

		FString LayerName = bSingleLayer ? GroupName : FString::Printf(TEXT("%s_%s"), *GroupName, *Layer.Name);

		// Disambiguate if the GLB has two nodes with the same name.
		FString UniqueLayerName = LayerName;
		int32 Suffix = 2;
		while (UsedNames.Contains(UniqueLayerName))
			UniqueLayerName = FString::Printf(TEXT("%s_%d"), *LayerName, Suffix++);
		UsedNames.Add(UniqueLayerName);

		AddOrReplaceMeshLayerAt(UniqueLayerName, Layer.Mesh, Layer.Transform);
		NewNames.Add(UniqueLayerName);
	}
}

void ATempUIActor::SetLayerGroupVisible(const FString& GroupName, bool bVisible)
{
	const TArray<FString>* Names = MeshLayerGroups.Find(GroupName);
	if (!Names || Names->IsEmpty())
		return;

	// Set every layer's visibility directly rather than calling SetMeshLayerVisible() per
	// layer — that would re-trace the terrain exclusion polygon once per layer in the group.
	for (const FString& LayerName : *Names)
		if (UStaticMeshComponent** Layer = MeshLayers.Find(LayerName))
			(*Layer)->SetVisibility(bVisible);

	OnMeshLayersChanged.Broadcast();

	if (TerrainExclusionPolygon)
	{
		RemoveTerrainExclusionPolygon();
		SpawnTerrainExclusionPolygon();
	}
}

bool ATempUIActor::IsLayerGroupVisible(const FString& GroupName) const
{
	const TArray<FString>* Names = MeshLayerGroups.Find(GroupName);
	if (!Names)
		return false;

	for (const FString& LayerName : *Names)
		if (GetMeshLayerVisible(LayerName))
			return true;

	return false;
}

bool ATempUIActor::HasLayerGroup(const FString& GroupName) const
{
	const TArray<FString>* Names = MeshLayerGroups.Find(GroupName);
	return Names && !Names->IsEmpty();
}

TArray<FString> ATempUIActor::GetMeshLayerNames() const
{
	TArray<FString> Names;
	MeshLayers.GetKeys(Names);
	return Names;
}

void ATempUIActor::SetMeshLayerVisible(const FString& LayerName, bool bVisible)
{
	if (UStaticMeshComponent** Layer = MeshLayers.Find(LayerName))
	{
		(*Layer)->SetVisibility(bVisible);
		OnMeshLayersChanged.Broadcast();

		// The exclusion shape tracks whichever layer is visible, so re-trace it on every swap.
		if (TerrainExclusionPolygon)
		{
			RemoveTerrainExclusionPolygon();
			SpawnTerrainExclusionPolygon();
		}
	}
}

bool ATempUIActor::GetMeshLayerVisible(const FString& LayerName) const
{
	if (const UStaticMeshComponent* const* Layer = MeshLayers.Find(LayerName))
		return (*Layer)->IsVisible();
	return false;
}

void ATempUIActor::SetMeshLayerTranslucent(const FString& LayerName, bool bTranslucent)
{
	UStaticMeshComponent** LayerPtr = MeshLayers.Find(LayerName);
	if (!LayerPtr)
		return;

	UStaticMeshComponent* Layer = *LayerPtr;

	if (bTranslucent)
	{
		if (OriginalLayerMaterials.Contains(LayerName))
			return; // already translucent

		const UMeshVisualSettings* Settings = GetDefault<UMeshVisualSettings>();
		UMaterialInterface* GhostMaterial = Settings ? Settings->GhostMaterial.LoadSynchronous() : nullptr;
		if (!GhostMaterial)
		{
			UE_LOG(LogTemp, Warning, TEXT("TempUIActor [%s]: no GhostMaterial configured in Project Settings > Mesh Visual Settings — transparency toggle has no effect"), *ThingId);
			return;
		}

		FMeshLayerMaterialSet& Cache = OriginalLayerMaterials.Add(LayerName);
		const int32 NumSlots = Layer->GetNumMaterials();
		Cache.Materials.Reserve(NumSlots);
		for (int32 i = 0; i < NumSlots; ++i)
		{
			Cache.Materials.Add(Layer->GetMaterial(i));
			Layer->SetMaterial(i, GhostMaterial);
		}
	}
	else
	{
		FMeshLayerMaterialSet Cache;
		if (!OriginalLayerMaterials.RemoveAndCopyValue(LayerName, Cache))
			return; // wasn't translucent

		for (int32 i = 0; i < Cache.Materials.Num(); ++i)
			Layer->SetMaterial(i, Cache.Materials[i]);
	}
}

bool ATempUIActor::GetMeshLayerTranslucent(const FString& LayerName) const
{
	return OriginalLayerMaterials.Contains(LayerName);
}

// ============================================================
//  Expiry — generic TTL from attributes.expiry_ts
// ============================================================

void ATempUIActor::TryApplyExpiry()
{
	if (!RawJson.IsValid())
		return;

	const TSharedPtr<FJsonObject> *AttribObj = nullptr;
	if (!RawJson->TryGetObjectField(TEXT("attributes"), AttribObj) || !AttribObj)
		return;

	FString ExpiryStr;
	if (!(*AttribObj)->TryGetStringField(TEXT("expiry_ts"), ExpiryStr) || ExpiryStr.IsEmpty())
		return;

	FDateTime ExpiryTime;
	if (!FDateTime::ParseIso8601(*ExpiryStr, ExpiryTime))
	{
		UE_LOG(LogTemp, Warning, TEXT("TempUIActor [%s]: failed to parse expiry_ts '%s'"), *ThingId, *ExpiryStr);
		return;
	}

	const float SecondsUntilExpiry = (float)(ExpiryTime - FDateTime::UtcNow()).GetTotalSeconds();
	if (SecondsUntilExpiry <= 0.0f)
	{
		Destroy();
		return;
	}

	GetWorldTimerManager().SetTimer(ExpiryTimer, this, &ATempUIActor::OnExpired, SecondsUntilExpiry, false);
	UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: expires in %.1fs"), *ThingId, SecondsUntilExpiry);
}

void ATempUIActor::OnExpired()
{
	UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: TTL elapsed, destroying"), *ThingId);
	Destroy();
}

// ============================================================
//  ApplyScale — resize mesh + interaction box from metric dimensions
// ============================================================

// ============================================================
//  Terrain exclusion — hides Cesium terrain tiles under the GLB model
// ============================================================

void ATempUIActor::SpawnTerrainExclusionPolygon()
{
	if (LastLatitude == 0.0 && LastLongitude == 0.0)
		return;

	// Destroy any previous polygon for this actor (e.g. after a model URL change)
	RemoveTerrainExclusionPolygon();

	// Use whichever mesh layer is currently visible (e.g. "Cone" vs "Simulation") so the
	// exclusion shape always matches what's actually on screen, not a fixed layer name.
	UStaticMeshComponent* MeshForBounds = nullptr;
	for (const TPair<FString, UStaticMeshComponent*>& Layer : MeshLayers)
	{
		if (Layer.Value && Layer.Value->IsVisible())
		{
			MeshForBounds = Layer.Value;
			break;
		}
	}
	if (!MeshForBounds)
		MeshForBounds = StaticMeshComponent;

	const FBox WorldBox = MeshForBounds->Bounds.GetBox();
	const float FloorZ  = WorldBox.Min.Z;

	// Small point-like models (streetlights, signs, ...) sit fine on top of terrain; excluding
	// terrain under them only wastes an overlay/tileset and can leave a visible pinhole. Only
	// carve out terrain for genuinely large-footprint meshes (fire cones, exclusion walls, etc).
	const FVector2D HorizontalExtent(WorldBox.GetExtent().X, WorldBox.GetExtent().Y);
	if (HorizontalExtent.Size() * 2.f < MinExclusionFootprintCm)
	{
		UE_LOG(LogTemp, Log,
			TEXT("TempUIActor [%s]: mesh footprint too small (%.0fcm) for terrain exclusion, skipping"),
			*ThingId, HorizontalExtent.Size() * 2.f);
		return;
	}

	// Target only the local photogrammetry (P3D) tileset — not the global terrain.
	// Global terrain tiles are very large and excluding whole tiles creates huge rectangular gaps.
	TArray<ACesium3DTileset*> AllTilesets;
	for (TActorIterator<ACesium3DTileset> It(GetWorld()); It; ++It)
	{
		if (!(*It)->GetActorNameOrLabel().Contains(TEXT("Terrain"), ESearchCase::IgnoreCase))
			AllTilesets.Add(*It);
	}

	if (AllTilesets.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("TempUIActor [%s]: no P3D tileset found, skipping exclusion polygon"), *ThingId);
		return;
	}

	// Spawn the polygon actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	TerrainExclusionPolygon = GetWorld()->SpawnActor<ACesiumCartographicPolygon>(
		ACesiumCartographicPolygon::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!TerrainExclusionPolygon)
		return;

	USplineComponent* Spline = TerrainExclusionPolygon->Polygon;
	Spline->ClearSplinePoints(false);

	bool bHullBuilt = false;

	// Primary (fire/ignition entities): use the exact GeoJSON perimeter matching whichever
	// layer is actually visible right now. This is the authoritative simulation output, not a
	// derived approximation, so prefer it over tracing the GLB mesh. Prefer the simulation
	// perimeter only when the Simulation layer is the one on screen; otherwise use the cone
	// perimeter.
	if (!bHullBuilt && DataStructType == FIgnitionPointData::StaticStruct())
	{
		const bool bSimulationVisible = IsLayerGroupVisible(TEXT("Simulation"));

		const TArray<FVector2D>* BestPoints = nullptr;
		if (bSimulationVisible)
		{
			for (int32 s = ParsedPerimeterSteps.Num() - 1; s >= 0; --s)
			{
				if (ParsedPerimeterSteps[s].Num() >= 3)
				{
					BestPoints = &ParsedPerimeterSteps[s];
					break;
				}
			}
		}
		if (!BestPoints && ParsedConePerimeterPoints.Num() >= 3)
			BestPoints = &ParsedConePerimeterPoints;
		if (!BestPoints)
		{
			for (int32 s = ParsedPerimeterSteps.Num() - 1; s >= 0; --s)
			{
				if (ParsedPerimeterSteps[s].Num() >= 3)
				{
					BestPoints = &ParsedPerimeterSteps[s];
					break;
				}
			}
		}

		if (BestPoints)
		{
			UCoordinatesConversionService *CoordSvc = UCoordinatesConversionService::Get();
			for (const FVector2D &Pt : *BestPoints) // X=lat, Y=lon
			{
				const FVector WorldPt = CoordSvc->ConvertWSG84ToUELocal(Pt.X, Pt.Y, 0.0);
				Spline->AddSplinePoint(FVector(WorldPt.X, WorldPt.Y, FloorZ), ESplineCoordinateSpace::World, false);
			}
			bHullBuilt = true;
		}
	}

	// Fallback: 2D convex hull of every mesh vertex projected onto the ground plane. This
	// covers non-fire entities (generic "Polygon" layer meshes) and any case where GeoJSON
	// perimeter data isn't available yet. A convex hull — rather than boundary-edge tracing —
	// is used because closed/watertight GLB solids (e.g. the cone model) have no true boundary
	// edges at all; the only "boundary" edges an edge-tracing pass would find belong to
	// unrelated open sub-geometry bundled in the same mesh (markers, decals, etc.), producing a
	// bogus small loop instead of the model's actual silhouette.
	if (!bHullBuilt)
	{
		UStaticMesh* SMesh = MeshForBounds->GetStaticMesh();
		if (SMesh && SMesh->GetRenderData() && SMesh->GetRenderData()->LODResources.Num() > 0)
		{
			const FStaticMeshLODResources& LOD = SMesh->GetRenderData()->LODResources[0];
			const FPositionVertexBuffer&   VB  = LOD.VertexBuffers.PositionVertexBuffer;
			const FTransform MeshTF = MeshForBounds->GetComponentTransform();
			const uint32 NumVerts = VB.GetNumVertices();

			TArray<FVector2D> Points;
			Points.Reserve(NumVerts);
			for (uint32 vi = 0; vi < NumVerts; vi++)
			{
				const FVector WP = MeshTF.TransformPosition(FVector(VB.VertexPosition(vi)));
				Points.Add(FVector2D(WP.X, WP.Y));
			}

			const TArray<FVector2D> Hull = ComputeConvexHull2D(Points);
			if (Hull.Num() >= 3)
			{
				for (const FVector2D& P : Hull)
					Spline->AddSplinePoint(FVector(P.X, P.Y, FloorZ), ESplineCoordinateSpace::World, false);
				bHullBuilt = true;
			}
		}
	}

	// Fallback: axis-aligned bounding box if mesh data was unavailable.
	if (!bHullBuilt)
	{
		const TArray<FVector> Corners = {
			FVector(WorldBox.Min.X, WorldBox.Min.Y, FloorZ),
			FVector(WorldBox.Max.X, WorldBox.Min.Y, FloorZ),
			FVector(WorldBox.Max.X, WorldBox.Max.Y, FloorZ),
			FVector(WorldBox.Min.X, WorldBox.Max.Y, FloorZ),
		};
		for (const FVector& C : Corners)
			Spline->AddSplinePoint(C, ESplineCoordinateSpace::World, false);
	}
	Spline->SetClosedLoop(true, false);
	for (int32 i = 0; i < Spline->GetNumberOfSplinePoints(); i++)
		Spline->SetSplinePointType(i, ESplinePointType::Linear, false);
	Spline->UpdateSpline();

	TSoftObjectPtr<ACesiumCartographicPolygon> SoftPolygon(TerrainExclusionPolygon);

	for (ACesium3DTileset* Tileset : AllTilesets)
	{
		// Find or create our named exclusion overlay on this tileset.
		UCesiumPolygonRasterOverlay* Overlay = nullptr;
		TArray<UCesiumPolygonRasterOverlay*> Overlays;
		Tileset->GetComponents<UCesiumPolygonRasterOverlay>(Overlays);
		for (UCesiumPolygonRasterOverlay* O : Overlays)
		{
			if (O->GetName().Equals(TEXT("DT4MOB_ExclusionOverlay_") + ThingId))
			{
				Overlay = O;
				break;
			}
		}
		if (!Overlay)
		{
			Overlay = NewObject<UCesiumPolygonRasterOverlay>(
				Tileset, *FString(TEXT("DT4MOB_ExclusionOverlay_") + ThingId));
			Overlay->ExcludeSelectedTiles = false;
			Tileset->AddInstanceComponent(Overlay);
			Overlay->RegisterComponent();
		}

		// Replace polygon list and force the overlay to rebuild its Cesium-side state.
		Overlay->Polygons = { SoftPolygon };
		Overlay->Deactivate();
		Overlay->Activate(true);

		UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: exclusion overlay applied to tileset '%s'"),
			*ThingId, *Tileset->GetActorNameOrLabel());
	}
}

void ATempUIActor::RemoveTerrainExclusionPolygon()
{
	if (!TerrainExclusionPolygon)
		return;

	const FString OverlayName = TEXT("DT4MOB_ExclusionOverlay_") + ThingId;
	for (TActorIterator<ACesium3DTileset> It(GetWorld()); It; ++It)
	{
		TArray<UCesiumPolygonRasterOverlay*> AllOverlays;
		(*It)->GetComponents<UCesiumPolygonRasterOverlay>(AllOverlays);
		for (UCesiumPolygonRasterOverlay* O : AllOverlays)
		{
			if (!O->GetName().Equals(OverlayName))
				continue;
			O->Deactivate();
			O->DestroyComponent();
			break;
		}
	}

	TerrainExclusionPolygon->Destroy();
	TerrainExclusionPolygon = nullptr;
}

// ============================================================
//  ApplyScale — resize mesh + interaction box from metric dimensions
// ============================================================

void ATempUIActor::ApplyScale(double LengthM, double WidthM, double HeightM)
{
	// UE default Cube mesh local bounds: ±50 UU → 100 UU = 1 m at scale 1.0.
	// Pass dimension in metres directly as scale factor.
	const FVector Scale(
		LengthM > 0.0 ? LengthM : 1.0,
		WidthM  > 0.0 ? WidthM  : 1.0,
		HeightM > 0.0 ? HeightM : 1.0
	);
	StaticMeshComponent->SetWorldScale3D(Scale);
	// Box half-extent = half the world size in UU (50 UU/m × dimension)
	InteractionBox->SetBoxExtent(FVector(
		(float)(Scale.X * 50.0),
		(float)(Scale.Y * 50.0),
		(float)(Scale.Z * 50.0)
	));
	bHasAppliedScale = true;
	UE_LOG(LogTemp, Verbose, TEXT("TempUIActor [%s]: scale %.2fx%.2fx%.2f m"),
		   *ThingId, LengthM, WidthM, HeightM);
}