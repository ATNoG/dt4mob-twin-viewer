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
#include "JsonObjectConverter.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Services/EntityUpdateDaemon.h"
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

	GetWorldTimerManager().SetTimer(VisibilityCheckTimer, this, &ATempUIActor::CheckVisibility, 1.0f, true);

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
	else if (Path == TEXT("/"))
	{
		// Full replace — swap entirely
		RawJson = ValueObject;
	}
	else
	{
		// Merge: write every field from the patch into RawJson
		for (const auto &Field : ValueObject->Values)
		{
			RawJson->SetField(Field.Key, Field.Value);
		}
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
	double DirectLat = 0.0, DirectLon = 0.0, DirectSpeedMs = 0.0;
	ValueObject->TryGetNumberField(TEXT("latitude"),  DirectLat);
	ValueObject->TryGetNumberField(TEXT("longitude"), DirectLon);
	ValueObject->TryGetNumberField(TEXT("speed"),     DirectSpeedMs);
	if (DirectLat != 0.0 || DirectLon != 0.0)
	{
		SetMovementTarget(DirectLat, DirectLon, DirectSpeedMs * 3.6, !bReceivedFirstLiveUpdate);
		bReceivedFirstLiveUpdate = true;
	}

	OnEntityDataChanged.Broadcast();
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

// ============================================================
//  RefreshFireExclusion — respawn exclusion polygon from fire perimeter data
// ============================================================

void ATempUIActor::RefreshFireExclusion()
{
	if (!StructInstance.IsValid() || DataStructType != FIgnitionPointData::StaticStruct()) return;

	const FIgnitionPointData* Data = reinterpret_cast<const FIgnitionPointData*>(StructInstance->GetStructMemory());
	if (Data->features.perimeters.properties.perimeters.IsEmpty()) return;

	// Only refresh if we already have an exclusion polygon (i.e. the GLB has loaded).
	// SpawnTerrainExclusionPolygon will pick up the perimeter data on first call too.
	if (TerrainExclusionPolygon)
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
	InterpolationSpeedKmh = SpeedKmh;

	const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	if (!bHasInterpolationTarget || bTeleport)
	{
		// Teleport the visual position — used on first-ever placement and on the
		// first live WebSocket update so stale REST data doesn't create a ghost trail.
		VisualLatitude  = Lat;
		VisualLongitude = Lon;
		bHasInterpolationTarget = true;
		GlobeAnchor->MoveToLongitudeLatitudeHeight(
			FVector(Lon, Lat, bSnappedToGround ? GlobeAnchor->GetHeight() : 0.0));
	}
	else if (LastTargetSetTime > 0.0)
	{
		// Blend toward a running average so brief bursts don't skew the estimate.
		const double Measured = Now - LastTargetSetTime;
		if (Measured > 0.05 && Measured < 10.0)
			EstimatedUpdateInterval = FMath::Lerp(EstimatedUpdateInterval, Measured, 0.25);
	}
	LastTargetSetTime = Now;

	LastLatitude  = Lat;
	LastLongitude = Lon;
	TriggerSnapIfNeeded();
}

// ============================================================
//  Tick — smoothly interpolate visual position toward server target
// ============================================================

void ATempUIActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bHasInterpolationTarget)
		return;

	const double dLat  = LastLatitude  - VisualLatitude;
	const double dLon  = LastLongitude - VisualLongitude;
	const double dLatM = dLat * 111000.0;
	const double dLonM = dLon * 111000.0 * FMath::Cos(FMath::DegreesToRadians(VisualLatitude));
	const double DistM = FMath::Sqrt(dLatM * dLatM + dLonM * dLonM);

	if (DistM > 0.05)
	{
		double t;
		const double SpeedMS = InterpolationSpeedKmh / 3.6;
		if (SpeedMS > 0.1)
		{
			// Speed-based: move at the vehicle's reported speed.
			t = FMath::Min((SpeedMS * DeltaTime) / DistM, 1.0);
		}
		else
		{
			// No speed in payload — spread the move over the estimated update interval.
			t = FMath::Min(DeltaTime / FMath::Max(EstimatedUpdateInterval, 0.1), 1.0);
		}

		VisualLatitude  += t * dLat;
		VisualLongitude += t * dLon;

		const double UseHeight = bSnappedToGround ? GlobeAnchor->GetHeight() : LastSnappedAltitudeMeters;
		GlobeAnchor->MoveToLongitudeLatitudeHeight(FVector(VisualLongitude, VisualLatitude, UseHeight));
	}

	// Rotate to face the direction of travel.
	// Only update heading when meaningfully moving to avoid jitter when nearly stopped.
	if (DistM > 0.5)
	{
		ACesiumGeoreference *GeoRef = ACesiumGeoreference::GetDefaultGeoreference(GetWorld());
		if (GeoRef)
		{
			// In Cesium's ESU frame: X=East, Y=South, Z=Up.
			// atan2(-dLatM, dLonM) gives the ESU yaw for a movement of (dEast, dNorth).
			const double EsuYawDeg = FMath::RadiansToDegrees(FMath::Atan2(-dLatM, dLonM));
			const FRotator WorldRot = GeoRef->TransformEastSouthUpRotatorToUnreal(
				FRotator(0.0, EsuYawDeg, 0.0), GetActorLocation());
			SetActorRotation(WorldRot);
		}
	}
}

// ============================================================
//  TriggerSnapIfNeeded — re-snap only when far enough from last snap point
// ============================================================

void ATempUIActor::TriggerSnapIfNeeded()
{
	if (bSnapInProgress)
		return;

	// ~50 m threshold in degrees — avoids re-snapping on every small position update.
	const double DLat = FMath::Abs(LastLatitude  - LastSnappedLatitude);
	const double DLon = FMath::Abs(LastLongitude - LastSnappedLongitude);
	if (DLat < 0.0005 && DLon < 0.0005)
		return;

	bSnappedToGround = false;
	if (!GetWorldTimerManager().IsTimerActive(VisibilityCheckTimer))
		GetWorldTimerManager().SetTimer(VisibilityCheckTimer, this, &ATempUIActor::CheckVisibility, 1.0f, true);
}

// ============================================================
//  CheckVisibility — frustum log (temporary diagnostic)
// ============================================================

void ATempUIActor::CheckVisibility()
{
	UWorld *World = GetWorld();
	APlayerController *PC = World->GetFirstPlayerController();
	if (!PC)
		return;

	// ---- Frustum check ----
	FVector2D ScreenPos;
	bool bProjected = UGameplayStatics::ProjectWorldToScreen(PC, GetActorLocation(), ScreenPos);
	if (!bProjected)
		return;

	int32 SX, SY;
	PC->GetViewportSize(SX, SY);
	bool bInFrustum = ScreenPos.X >= 0 && ScreenPos.X <= SX &&
					  ScreenPos.Y >= 0 && ScreenPos.Y <= SY;
	if (!bInFrustum)
		return;

	// ---- Geographic distance check ----
	ACesiumGeoreference *GeoRef = ACesiumGeoreference::GetDefaultGeoreference(World);
	if (!GeoRef)
		return;

	FVector CamUEPos;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamUEPos, CamRot);

	FVector CamLLH = GeoRef->TransformUnrealPositionToLongitudeLatitudeHeight(CamUEPos);
	// CamLLH.X = Longitude, CamLLH.Y = Latitude

	double dLat = (LastLatitude  - CamLLH.Y) * 111000.0;
	double dLon = (LastLongitude - CamLLH.X) * 111000.0 * FMath::Cos(FMath::DegreesToRadians(LastLatitude));
	double DistMetres = FMath::Sqrt(dLat * dLat + dLon * dLon);

	constexpr double MaxDistMetres = 5000.0;
	if (DistMetres > MaxDistMetres)
		return;

	UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: IN VIEW dist=%.0fm (screen %.0f, %.0f)"),
		   *ThingId, DistMetres, ScreenPos.X, ScreenPos.Y);

	SnapToGround();
}

// ============================================================
//  SnapToGround — deferred terrain-surface placement
// ============================================================

void ATempUIActor::SnapToGround()
{
	UWorld *World = GetWorld();
	if (!World || (LastLatitude == 0.0 && LastLongitude == 0.0))
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

	// Line trace — hits any geometry except CWT and self.
	// Accepts P3D tiles (if collision enabled), testtalude, or any static mesh.
	{
		FVector TraceStart = UCoordinatesConversionService::Get()->ConvertWSG84ToUELocal(LastLatitude, LastLongitude, 5000.0);
		FVector TraceEnd   = UCoordinatesConversionService::Get()->ConvertWSG84ToUELocal(LastLatitude, LastLongitude, -500.0);

		TArray<FHitResult> Hits;
		FCollisionQueryParams Params(NAME_None, /*bTraceComplex=*/true);
		Params.AddIgnoredActor(this);

		// ObjectType query returns ALL hits without stopping at the first blocker,
		// so we get both CWT (above) and P3D (below it in road-cut sections).
		FCollisionObjectQueryParams ObjectParams(ECC_WorldStatic);
		World->LineTraceMultiByObjectType(Hits, TraceStart, TraceEnd, ObjectParams, Params);

		// Pick the hit with the lowest world Z across all actors — instruments may sit
		// on non-Cesium geometry (bridges, walls) so we don't filter by actor type.
		FHitResult *BestHit = nullptr;
		for (FHitResult &Hit : Hits)
		{
			if (!BestHit || Hit.ImpactPoint.Z < BestHit->ImpactPoint.Z)
				BestHit = &Hit;
		}

		UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: trace %d hits, best: %s"),
			   *ThingId, Hits.Num(), BestHit ? *GetNameSafe(BestHit->GetActor()) : TEXT("none"));

		if (BestHit)
		{
			ACesiumGeoreference *GeoRef = ACesiumGeoreference::GetDefaultGeoreference(World);
			if (GeoRef)
			{
				FVector LLH = GeoRef->TransformUnrealPositionToLongitudeLatitudeHeight(BestHit->Location);
				GlobeAnchor->MoveToLongitudeLatitudeHeight(LLH);
				LastSnappedAltitudeMeters = LLH.Z;
			}
			else
			{
				SetActorLocation(BestHit->Location);
			}
			bSnappedToGround = true;
			LastSnappedLatitude  = LastLatitude;
			LastSnappedLongitude = LastLongitude;
			GetWorldTimerManager().ClearTimer(VisibilityCheckTimer);
			UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: snapped to mesh surface '%s'"),
				   *ThingId, *GetNameSafe(BestHit->GetActor()));
			return;
		}
	}

	// P3D tiles not yet loaded or no collision — use CWT async as a temporary position.
	// CheckVisibility will retry the trace next second.
	if (!TerrainTileset)
		return;

	TArray<FVector> Positions = {FVector(LastLongitude, LastLatitude, 0.0)};
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
		UE_LOG(LogTemp, Warning, TEXT("TempUIActor [%s]: height sample returned no results"), *ThingId);
		return;
	}

	const FCesiumSampleHeightResult &Result = Results[0];
	if (!Result.SampleSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("TempUIActor [%s]: height sample failed (no tile coverage at this position)"), *ThingId);
		return;
	}

	// Use MoveToLongitudeLatitudeHeight so the GlobeAnchor's internal LLH cache
	// is updated — SetActorLocation alone can be overridden on the next origin rebase.
	GlobeAnchor->MoveToLongitudeLatitudeHeight(Result.LongitudeLatitudeHeight);
	bSnappedToGround = true;
	bSnapInProgress = false;
	LastSnappedAltitudeMeters  = Result.LongitudeLatitudeHeight.Z;
	LastSnappedLatitude  = LastLatitude;
	LastSnappedLongitude = LastLongitude;
	GetWorldTimerManager().ClearTimer(VisibilityCheckTimer);

	UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: snapped to CWT ground — sampled height=%.1fm"),
		   *ThingId, Result.LongitudeLatitudeHeight.Z);
}

// ============================================================
//  GLB model loading
// ============================================================

void ATempUIActor::TryLoadGlbModel()
{
	if (!RawJson.IsValid())
		return;

	const TSharedPtr<FJsonObject> *AttribObj = nullptr;
	if (!RawJson->TryGetObjectField(TEXT("attributes"), AttribObj) || !AttribObj)
		return;

	FString PolygonUrl;
	if (!(*AttribObj)->TryGetStringField(TEXT("polygon"), PolygonUrl) || PolygonUrl.IsEmpty())
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

	FOnGlbMeshLoaded Callback;
	Callback.BindDynamic(this, &ATempUIActor::OnPolygonMeshLoaded);
	Svc->RequestMesh(PolygonUrl, Callback);
}

void ATempUIActor::OnPolygonMeshLoaded(UStaticMesh *Mesh)
{
	if (!Mesh)
	{
		UE_LOG(LogTemp, Warning, TEXT("TempUIActor [%s]: GLB mesh load failed"), *ThingId);
		return;
	}

	StaticMeshComponent->SetStaticMesh(Mesh);
	UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: GLB mesh applied"), *ThingId);

	SpawnTerrainExclusionPolygon();
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

	const FBox WorldBox = StaticMeshComponent->Bounds.GetBox();
	const float FloorZ  = WorldBox.Min.Z;

	USplineComponent* Spline = TerrainExclusionPolygon->Polygon;
	Spline->ClearSplinePoints(false);

	bool bHullBuilt = false;

	// Primary: trace the actual mesh boundary edges (edges shared by exactly one triangle).
	// This produces the exact silhouette of the mesh in XY, with no inflation or approximation.
	if (!bHullBuilt)
	{
		UStaticMesh* SMesh = StaticMeshComponent->GetStaticMesh();
		if (SMesh && SMesh->GetRenderData() && SMesh->GetRenderData()->LODResources.Num() > 0)
		{
			const FStaticMeshLODResources& LOD = SMesh->GetRenderData()->LODResources[0];
			const FPositionVertexBuffer&   VB  = LOD.VertexBuffers.PositionVertexBuffer;
			const FRawStaticIndexBuffer&   IB  = LOD.IndexBuffer;
			const uint32 NumIndices = IB.GetNumIndices();
			const FTransform MeshTF = StaticMeshComponent->GetComponentTransform();

			if (NumIndices >= 3)
			{
				// Count how many triangles reference each undirected edge.
				TMap<TPair<uint32,uint32>, int32> EdgeCount;
				EdgeCount.Reserve(NumIndices);
				for (uint32 i = 0; i + 2 < NumIndices; i += 3)
				{
					uint32 v[3] = { IB.GetIndex(i), IB.GetIndex(i+1), IB.GetIndex(i+2) };
					for (int32 e = 0; e < 3; e++)
					{
						uint32 A = v[e], B = v[(e+1)%3];
						EdgeCount.FindOrAdd(TPair<uint32,uint32>(FMath::Min(A,B), FMath::Max(A,B)))++;
					}
				}

				// Collect directed boundary edges (count == 1 → on the outer hull).
				TMap<uint32, uint32> BoundaryNext;
				for (uint32 i = 0; i + 2 < NumIndices; i += 3)
				{
					uint32 v[3] = { IB.GetIndex(i), IB.GetIndex(i+1), IB.GetIndex(i+2) };
					for (int32 e = 0; e < 3; e++)
					{
						uint32 A = v[e], B = v[(e+1)%3];
						if (EdgeCount[TPair<uint32,uint32>(FMath::Min(A,B), FMath::Max(A,B))] == 1)
							BoundaryNext.Add(A, B);
					}
				}

				if (BoundaryNext.Num() >= 3)
				{
					// Trace all boundary loops, then keep the one with the largest 2D bounding area.
					TArray<TArray<FVector2D>> Loops;
					TSet<uint32> Visited;
					for (auto& StartKV : BoundaryNext)
					{
						if (Visited.Contains(StartKV.Key)) continue;
						TArray<FVector2D> Loop;
						uint32 Cur = StartKV.Key;
						int32  Guard = BoundaryNext.Num() + 1;
						while (!Visited.Contains(Cur) && Guard-- > 0)
						{
							Visited.Add(Cur);
							const FVector3f LP = VB.VertexPosition(Cur);
							const FVector   WP = MeshTF.TransformPosition(FVector(LP));
							Loop.Add(FVector2D(WP.X, WP.Y));
							uint32* Next = BoundaryNext.Find(Cur);
							if (!Next) break;
							Cur = *Next;
						}
						if (Loop.Num() >= 3)
							Loops.Add(MoveTemp(Loop));
					}

					if (Loops.Num() > 0)
					{
						// Pick largest loop by bounding-box area.
						int32 Best = 0;
						float BestArea = 0.f;
						for (int32 L = 0; L < Loops.Num(); L++)
						{
							FBox2D Box(ForceInit);
							for (const FVector2D& P : Loops[L]) Box += P;
							const float A = Box.GetArea();
							if (A > BestArea) { BestArea = A; Best = L; }
						}

						for (const FVector2D& P : Loops[Best])
							Spline->AddSplinePoint(FVector(P.X, P.Y, FloorZ), ESplineCoordinateSpace::World, false);
						bHullBuilt = true;
					}
				}
			}
		}
	}

	// Fallback: use the outermost fire simulation perimeter ring (geographic coordinates).
	if (!bHullBuilt && DataStructType == FIgnitionPointData::StaticStruct() && StructInstance.IsValid())
	{
		const FIgnitionPointData* FireData = reinterpret_cast<const FIgnitionPointData*>(StructInstance->GetStructMemory());
		const TArray<FFireShape>& Perims = FireData->features.perimeters.properties.perimeters;
		if (!Perims.IsEmpty() && Perims.Last().points.Num() >= 3)
		{
			UCoordinatesConversionService* CoordSvc = UCoordinatesConversionService::Get();
			for (const FFireGeoPoint& Pt : Perims.Last().points)
			{
				const FVector WorldPt = CoordSvc->ConvertWSG84ToUELocal(Pt.lat, Pt.lon, 0.0);
				Spline->AddSplinePoint(FVector(WorldPt.X, WorldPt.Y, FloorZ), ESplineCoordinateSpace::World, false);
			}
			bHullBuilt = true;
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
	UE_LOG(LogTemp, Verbose, TEXT("TempUIActor [%s]: scale %.2fx%.2fx%.2f m"),
		   *ThingId, LengthM, WidthM, HeightM);
}