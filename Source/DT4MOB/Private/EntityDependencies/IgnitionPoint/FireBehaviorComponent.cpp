// Fill out your copyright notice in the Description page of Project Settings.

#include "EntityDependencies/IgnitionPoint/FireBehaviorComponent.h"
#include "Entities/TempUIActor.h"
#include "EntityStructs/IgnitionPointStruct.h"
#include "GeometryUtils.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Misc/Paths.h"
#include "Engine/GameInstance.h"

void UFireBehaviorComponent::OnEntityInitialized()
{
    OnEntityDataChanged();
}

void UFireBehaviorComponent::OnEntityDataChanged()
{
    TryLoadFireGlbModels(); // loads any new GLB URLs; also fetches perimeters at its tail

    ATempUIActor* Owner = GetOwnerEntity();
    if (Owner && Owner->HasTerrainExclusionPolygon() &&
        (!ParsedConePerimeterPoints.IsEmpty() || !ParsedPerimeterSteps.IsEmpty()))
    {
        Owner->RebuildTerrainExclusionPolygon();
    }
}

// ─── Multi-model GLB loading ─────────────────────────────────────────────────

void UFireBehaviorComponent::TryLoadFireGlbModels()
{
    ATempUIActor* Owner = GetOwnerEntity();
    if (!Owner || !Owner->RawJson.IsValid())
        return;

    const TSharedPtr<FJsonObject> *AttribObj = nullptr;
    if (!Owner->RawJson->TryGetObjectField(TEXT("attributes"), AttribObj) || !AttribObj)
        return;

    const TArray<TSharedPtr<FJsonValue>> *PolygonArr = nullptr;
    if (!(*AttribObj)->TryGetArrayField(TEXT("polygon"), PolygonArr) || !PolygonArr || PolygonArr->IsEmpty())
        return;

    UGameInstance *GI = Owner->GetGameInstance();
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
            Callback.BindDynamic(this, &UFireBehaviorComponent::OnConeGlbLayersLoaded);
        else
            Callback.BindDynamic(this, &UFireBehaviorComponent::OnSimulationGlbLayersLoaded);

        Svc->RequestMeshLayers(Url, Callback);
    }

    TryFetchFirePerimeters();
}

void UFireBehaviorComponent::OnConeGlbLayersLoaded(const TArray<FGlbMeshLayer>& GlbLayers)
{
    ATempUIActor* Owner = GetOwnerEntity();
    if (!Owner)
        return;

    if (GlbLayers.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("FireBehaviorComponent [%s]: cone GLB failed to load"), *Owner->GetThingId());
        return;
    }
    Owner->AddOrReplaceMeshLayerGroup(TEXT("Cone"), GlbLayers);
    Owner->StaticMeshComponent->SetVisibility(false);
    UE_LOG(LogTemp, Log, TEXT("FireBehaviorComponent [%s]: cone GLB loaded (%d mesh layer(s))"), *Owner->GetThingId(), GlbLayers.Num());
    Owner->RebuildTerrainExclusionPolygon();
}

void UFireBehaviorComponent::OnSimulationGlbLayersLoaded(const TArray<FGlbMeshLayer>& GlbLayers)
{
    ATempUIActor* Owner = GetOwnerEntity();
    if (!Owner)
        return;

    if (GlbLayers.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("FireBehaviorComponent [%s]: simulation GLB failed to load"), *Owner->GetThingId());
        return;
    }
    Owner->AddOrReplaceMeshLayerGroup(TEXT("Simulation"), GlbLayers);
    if (bSimulationStepsReady)
    {
        // Steps already completed before the GLB arrived — switch now.
        Owner->SetLayerGroupVisible(TEXT("Simulation"), true);
        Owner->SetLayerGroupVisible(TEXT("Cone"), false);
        UE_LOG(LogTemp, Log, TEXT("FireBehaviorComponent [%s]: simulation GLB loaded (%d mesh layer(s), steps already ready, switching)"), *Owner->GetThingId(), GlbLayers.Num());
    }
    else
    {
        Owner->SetLayerGroupVisible(TEXT("Simulation"), false);
        UE_LOG(LogTemp, Log, TEXT("FireBehaviorComponent [%s]: simulation GLB loaded (%d mesh layer(s), hidden until steps ready)"), *Owner->GetThingId(), GlbLayers.Num());
    }

    // SetLayerGroupVisible() above only re-traces an existing polygon; make sure one exists
    // even if the cone GLB never loaded (e.g. it failed or this entity has no cone model).
    if (!Owner->HasTerrainExclusionPolygon())
        Owner->RebuildTerrainExclusionPolygon();
}

// ─── GeoJSON perimeter fetching ──────────────────────────────────────────────

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

TArray<FVector2D> UFireBehaviorComponent::ParseGeoJsonOuterRing(const FString &JsonStr)
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
TArray<FVector2D> UFireBehaviorComponent::ParseConeGeoJsonHull(const FString &JsonStr)
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

    return FGeometryUtils::ComputeConvexHull2D(AllPoints);
}

void UFireBehaviorComponent::TryFetchFirePerimeters()
{
    ATempUIActor* Owner = GetOwnerEntity();
    if (!Owner || !Owner->StructInstance.IsValid())
        return;

    const FIgnitionPointData *Data =
        reinterpret_cast<const FIgnitionPointData *>(Owner->StructInstance->GetStructMemory());

    // ── Cone GeoJSON ──────────────────────────────────────────────────────────
    const FString &ConeUrl = Data->features.cone.properties.perimeters;
    if (!ConeUrl.IsEmpty() && ConeUrl != FetchedConeGeoJsonUrl)
    {
        FetchedConeGeoJsonUrl = ConeUrl;
        TWeakObjectPtr<UFireBehaviorComponent> WeakThis(this);
        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
        Req->SetURL(ConeUrl);
        Req->SetVerb(TEXT("GET"));
        Req->OnProcessRequestComplete().BindLambda(
            [WeakThis](FHttpRequestPtr, FHttpResponsePtr Response, bool bOk)
            {
                if (!WeakThis.IsValid() || !bOk || !Response.IsValid() || Response->GetResponseCode() != 200)
                    return;
                UFireBehaviorComponent *Self = WeakThis.Get();
                ATempUIActor *SelfOwner = Self->GetOwnerEntity();
                if (!SelfOwner)
                    return;
                Self->ParsedConePerimeterPoints = ParseConeGeoJsonHull(Response->GetContentAsString());
                UE_LOG(LogTemp, Log, TEXT("FireBehaviorComponent [%s]: cone perimeter parsed (%d pts)"),
                    *SelfOwner->GetThingId(), Self->ParsedConePerimeterPoints.Num());
                if (SelfOwner->HasTerrainExclusionPolygon())
                    SelfOwner->RebuildTerrainExclusionPolygon();
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
    TWeakObjectPtr<UFireBehaviorComponent> WeakThis(this);

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
                UFireBehaviorComponent *Self = WeakThis.Get();
                ATempUIActor *SelfOwner = Self->GetOwnerEntity();
                if (!SelfOwner)
                    return;

                if (bOk && Response.IsValid() && Response->GetResponseCode() == 200)
                {
                    TArray<FVector2D> Points = ParseGeoJsonOuterRing(Response->GetContentAsString());
                    if (Self->ParsedPerimeterSteps.IsValidIndex(Idx))
                        Self->ParsedPerimeterSteps[Idx] = MoveTemp(Points);
                    UE_LOG(LogTemp, Log, TEXT("FireBehaviorComponent [%s]: step %d min parsed (%d pts)"),
                        *SelfOwner->GetThingId(), StepMin, Self->ParsedPerimeterSteps[Idx].Num());
                }

                if (--(*Remaining) == 0)
                {
                    Self->bSimulationStepsReady = true;
                    // Only switch visibility if the simulation GLB is already loaded.
                    // SetLayerGroupVisible() below re-traces the exclusion polygon for us.
                    if (SelfOwner->HasLayerGroup(TEXT("Simulation")))
                    {
                        SelfOwner->SetLayerGroupVisible(TEXT("Simulation"), true);
                        SelfOwner->SetLayerGroupVisible(TEXT("Cone"), false);
                        UE_LOG(LogTemp, Log, TEXT("FireBehaviorComponent [%s]: switched to simulation model"), *SelfOwner->GetThingId());
                    }
                    else
                    {
                        UE_LOG(LogTemp, Log, TEXT("FireBehaviorComponent [%s]: steps ready, waiting for simulation GLB"), *SelfOwner->GetThingId());
                    }
                }
            });
        Req->ProcessRequest();
    }
}

// ─── Exclusion polygon points ────────────────────────────────────────────────

bool UFireBehaviorComponent::GetExclusionPolygonPoints(TArray<FVector2D>& OutPoints) const
{
    ATempUIActor* Owner = GetOwnerEntity();
    if (!Owner)
        return false;

    // Prefer the simulation perimeter only when the Simulation layer is the one on screen;
    // otherwise use the cone perimeter.
    const bool bSimulationVisible = Owner->IsLayerGroupVisible(TEXT("Simulation"));

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

    if (!BestPoints)
        return false;

    OutPoints = *BestPoints;
    return true;
}
