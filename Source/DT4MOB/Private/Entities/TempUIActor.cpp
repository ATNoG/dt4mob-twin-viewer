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
#include "Managers/SelectionManager.h"
#include "Services/CoordinatesConversionService.h"
#include "CesiumSampleHeightMostDetailedAsyncAction.h"
#include "Cesium3DTileset.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "CesiumGeoreference.h"
#include "DrawDebugHelpers.h"

// ============================================================
//  Construction
// ============================================================

ATempUIActor::ATempUIActor()
{
	PrimaryActorTick.bCanEverTick = false;

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
			{
				Daemon->UnregisterEntity(ThingId, OnEntityUpdated);
			}
		}
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
	SetActorLabel(*ThingId);
	UE_LOG(LogTemp, Log, TEXT("TempUIActor initialized: [%s]"), *ThingId);

	SetLocation();

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
		else
		{
			// "/features/<name>" or "/features/<name>/properties" — merge at feature level
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
	}

	OnEntityDataChanged.Broadcast();
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
					if (LocIt->GetName().Equals(TEXT("latitude"), ESearchCase::IgnoreCase))
					{
						if (FDoubleProperty *P = CastField<FDoubleProperty>(*LocIt))
							Location.X = *(double *)V;
					}
					else if (LocIt->GetName().Equals(TEXT("longitude"), ESearchCase::IgnoreCase))
					{
						if (FDoubleProperty *P = CastField<FDoubleProperty>(*LocIt))
							Location.Y = *(double *)V;
					}
				}
				break;
			}

			if (AttrName.Equals(TEXT("latitude"), ESearchCase::IgnoreCase))
			{
				if (FDoubleProperty *P = CastField<FDoubleProperty>(*AttrIt))
					Location.X = *(double *)P->ContainerPtrToValuePtr<void>(AttribPtr);
			}
			else if (AttrName.Equals(TEXT("longitude"), ESearchCase::IgnoreCase))
			{
				if (FDoubleProperty *P = CastField<FDoubleProperty>(*AttrIt))
					Location.Y = *(double *)P->ContainerPtrToValuePtr<void>(AttribPtr);
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

	// Position changed or not yet snapped — move to altitude 0 and re-trigger snap.
	if (bMoved)
	{
		bSnappedToGround = false;
		if (!GetWorldTimerManager().IsTimerActive(VisibilityCheckTimer))
			GetWorldTimerManager().SetTimer(VisibilityCheckTimer, this, &ATempUIActor::CheckVisibility, 1.0f, true);
	}

	FVector UELoc = UCoordinatesConversionService::Get()->ConvertWSG84ToUELocal(Location.X, Location.Y, 0.0);
	SetActorLocation(UELoc);
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
		const FString Label = Candidate->GetActorLabel();
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

		// Last hit = deepest/lowest surface = road or P3D level, not the cliff top.
		FHitResult *BestHit = Hits.IsEmpty() ? nullptr : &Hits.Last();

		UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: trace %d hits, best: %s"),
			   *ThingId, Hits.Num(), BestHit ? *GetNameSafe(BestHit->GetActor()) : TEXT("none"));

		if (BestHit)
		{
			ACesiumGeoreference *GeoRef = ACesiumGeoreference::GetDefaultGeoreference(World);
			if (GeoRef)
			{
				FVector LLH = GeoRef->TransformUnrealPositionToLongitudeLatitudeHeight(BestHit->Location);
				GlobeAnchor->MoveToLongitudeLatitudeHeight(LLH);
			}
			else
			{
				SetActorLocation(BestHit->Location);
			}
			bSnappedToGround = true;
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
	GetWorldTimerManager().ClearTimer(VisibilityCheckTimer);

	UE_LOG(LogTemp, Log, TEXT("TempUIActor [%s]: snapped to CWT ground — sampled height=%.1fm"),
		   *ThingId, Result.LongitudeLatitudeHeight.Z);
}