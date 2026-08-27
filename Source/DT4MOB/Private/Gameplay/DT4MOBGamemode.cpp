// Fill out your copyright notice in the Description page of Project Settings.

/** @file DT4MOBGamemode.cpp
 *  @brief Implementation of ADT4MOBGamemode. All logic documentation is in the header.
 */
#include "Gameplay/DT4MOBGamemode.h"
#include "Services/DittoService.h"
#include "Services/WSService.h"
#include "Services/EntityUpdateDaemon.h"
#include "Json.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "Gameplay/UnifiedPawn/UnifiedPawn.h"
#include "CesiumGeoreference.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"

ADT4MOBGamemode::ADT4MOBGamemode()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ADT4MOBGamemode::BeginPlay()
{
    Super::BeginPlay();

    UWorld *World = GetWorld();
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("ADT4MOBGamemode::BeginPlay: World is null"));
        return;
    }

    UGameInstance *GameInstance = GetGameInstance();
    if (!GameInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("ADT4MOBGamemode::BeginPlay: GameInstance is null"));
        return;
    }

    // ---- 1. Subscribe to unhandled WS messages for on-demand spawning -----
    if (UEntityUpdateDaemon *Daemon = GameInstance->GetSubsystem<UEntityUpdateDaemon>())
    {
        Daemon->OnUnhandledThingMessage.AddDynamic(this, &ADT4MOBGamemode::HandleUnhandledThingMessage);
    }

    // ---- 2. When tile-based streaming is disabled (see bUseTileStreaming), fully fetch
    //         every type whose extension opts in via RequiresFullFetch() (e.g. road signs)
    //         once, unfiltered by geotile — see EntityDependencies/*/*.h.
    if (!bUseTileStreaming)
    {
        UDittoService *DittoSvc = GameInstance->GetSubsystem<UDittoService>();
        UDT4MOBEntityFactory *Factory = GameInstance->GetSubsystem<UDT4MOBEntityFactory>();
        if (DittoSvc && Factory)
        {
            for (const FString &TypeKey : Factory->GetRegisteredTypeKeys())
            {
                UEntityTypeExtension *Extension = Factory->GetExtensionForType(TypeKey);
                if (!Extension->RequiresFullFetch())
                    continue;

                DittoSvc->GetAllThings(
                    [this, World](const TArray<TSharedPtr<FJsonObject>> &Page)
                    {
                        AsyncTask(ENamedThreads::GameThread, [this, World, Page]()
                        {
                            if (!IsValid(this) || !IsValid(World)) return;
                            if (UGameInstance *GI2 = GetGameInstance())
                            {
                                if (UDT4MOBEntityFactory *F = GI2->GetSubsystem<UDT4MOBEntityFactory>())
                                {
                                    for (const auto &Thing : Page)
                                        F->SpawnTempUIActor(World, Thing);
                                }
                            }
                        });
                    },
                    [TypeKey]()
                    {
                        UE_LOG(LogTemp, Log, TEXT("GetAllThings: full fetch for type '%s' complete"), *TypeKey);
                    },
                    Extension->GetFullFetchFilter(TypeKey));
            }
        }
    }
}

void ADT4MOBGamemode::HandleUnhandledThingMessage(const FString &ThingId, const FString &Path, const FString &ValueJson)
{
    // Entities are loaded exclusively via tile fetch by default — WS events for things not
    // already in the world are ignored here UNLESS the type explicitly opts in via
    // UEntityTypeExtension::AllowsOnDemandSpawnFromWS() (currently just toll vehicle-detection
    // sensors — see UTollCameraExtension — whose things are too short-lived for the periodic
    // tile-fetch/search index to reliably catch).
    UGameInstance *GI = GetGameInstance();
    if (!GI) return;

    UDT4MOBEntityFactory *Factory = GI->GetSubsystem<UDT4MOBEntityFactory>();
    if (!Factory || !Factory->CanHandleThingId(ThingId)) return;
    if (!Factory->GetExtensionForThingId(ThingId)->AllowsOnDemandSpawnFromWS()) return;

    if (PendingOnDemandSpawns.Contains(ThingId)) return; // already fetching for this thingId
    PendingOnDemandSpawns.Add(ThingId);

    UDittoService *DittoSvc = GI->GetSubsystem<UDittoService>();
    if (!DittoSvc)
    {
        PendingOnDemandSpawns.Remove(ThingId);
        return;
    }

    UWorld *World = GetWorld();
    DittoSvc->GetThingById(ThingId,
        [this, World, ThingId, Path, ValueJson](TSharedPtr<FJsonObject> ThingData)
        {
            AsyncTask(ENamedThreads::GameThread, [this, World, ThingId, Path, ValueJson, ThingData]()
            {
                if (!IsValid(this)) return;
                PendingOnDemandSpawns.Remove(ThingId);

                if (!IsValid(World) || !ThingData.IsValid()) return;
                UGameInstance *GI2 = GetGameInstance();
                if (!GI2) return;

                if (UDT4MOBEntityFactory *F = GI2->GetSubsystem<UDT4MOBEntityFactory>())
                    F->SpawnTempUIActor(World, ThingData);

                // Replay the update that triggered this spawn so the actor reflects the
                // freshest state immediately, rather than waiting for the next WS event.
                if (UEntityUpdateDaemon *Daemon = GI2->GetSubsystem<UEntityUpdateDaemon>())
                    Daemon->InjectUpdate(ThingId, Path, ValueJson);
            });
        });
}

void ADT4MOBGamemode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // The Daemon's Deinitialize() handles socket teardown automatically —
    // nothing to clean up here.
    Super::EndPlay(EndPlayReason);
}

void ADT4MOBGamemode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bUseTileStreaming)
    {
        TileCheckTimer -= DeltaSeconds;
        if (TileCheckTimer <= 0.f)
        {
            TileCheckTimer = TileCheckInterval;
            CheckAndRefreshTiles(TileCheckInterval);
        }
    }

    OrphanSweepTimer -= DeltaSeconds;
    if (OrphanSweepTimer <= 0.f)
    {
        OrphanSweepTimer = OrphanSweepInterval;
        if (UGameInstance* GI = GetGameInstance())
            if (UDT4MOBEntityFactory* Factory = GI->GetSubsystem<UDT4MOBEntityFactory>())
                Factory->SweepOrphanedActors();
    }
}

void ADT4MOBGamemode::CheckAndRefreshTiles(float DeltaSeconds)
{
    APlayerController *PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (!PC) return;

    AUnifiedPawn *Pawn = Cast<AUnifiedPawn>(PC->GetPawn());
    if (!Pawn) return;

    ACesiumGeoreference *Georeference = ACesiumGeoreference::GetDefaultGeoreferenceForActor(Pawn);
    if (!Georeference) return;

    const FVector LLH = Georeference->TransformUnrealPositionToLongitudeLatitudeHeight(Pawn->GetActorLocation());
    const double Lng = LLH.X;
    const double Lat = LLH.Y;
    const double CameraAltMeters = LLH.Z + Pawn->GetTargetArmLength() / 100.0;

    const int32 Zoom = UDittoService::AltitudeToZoomLevel(CameraAltMeters);
    if (Zoom < MinZoomForTileFiltering)
    {
        // Zoomed out past the tile threshold — remove geotile filter so all events flow through.
        if (UGameInstance* GI = GetGameInstance())
            if (UWSService* WSSvc = GI->GetSubsystem<UWSService>())
                WSSvc->SetEventFilter(TEXT(""));
        return;
    }

    const TSet<int64> NewKeys = GetNeighborTileKeys(Lat, Lng, Zoom);

    // Nothing changed — already loaded exactly this set at this zoom
    if (Zoom == LoadedZoom && NewKeys.Difference(LoadedTileKeys).Num() == 0 && LoadedTileKeys.Difference(NewKeys).Num() == 0)
        return;

    if (Zoom == PendingZoom && bPendingTileRefresh
        && NewKeys.Difference(PendingTileKeys).Num() == 0
        && PendingTileKeys.Difference(NewKeys).Num() == 0)
    {
        TileRefreshTimer -= DeltaSeconds;
        if (TileRefreshTimer <= 0.f)
        {
            bPendingTileRefresh = false;
            DoTileRefresh(PendingTileKeys, PendingZoom);
        }
    }
    else
    {
        PendingTileKeys     = NewKeys;
        PendingZoom         = Zoom;
        bPendingTileRefresh = true;
        TileRefreshTimer    = TileRefreshDelay;
    }
}

void ADT4MOBGamemode::DoTileRefresh(const TSet<int64>& NewTileKeys, int32 Zoom)
{
    UGameInstance *GI = GetGameInstance();
    if (!GI) return;

    UDT4MOBEntityFactory *Factory  = GI->GetSubsystem<UDT4MOBEntityFactory>();
    UDittoService        *DittoSvc = GI->GetSubsystem<UDittoService>();
    UWSService           *WSSvc    = GI->GetSubsystem<UWSService>();
    if (!Factory || !DittoSvc) return;

    // Update WebSocket event subscription to match the new visible tile set.
    //
    // Different Ditto data sources encode attributes/geotile at different zoom precisions —
    // TRACI-simulated vehicles at zoom 18 (verified against a live vehicle payload), static
    // infrastructure imports at zoom 31. OR both precisions' ranges together so live updates
    // from either source pass the filter, regardless of which type a given tile holds.
    if (WSSvc)
    {
        TArray<FString> Conditions;
        for (int64 Key : NewTileKeys)
        {
            for (int32 StorageZoom : GeotileStorageZooms)
            {
                int64 L, U;
                UDittoService::GetTileBoundsFromKey(Key, Zoom, L, U, StorageZoom);
                // Upper is the exclusive start of the next tile's range — lt, not le, avoids
                // double-matching entities sitting exactly on a tile boundary.
                Conditions.Add(FString::Printf(
                    TEXT("and(ge(attributes/geotile,%lld),lt(attributes/geotile,%lld))"), L, U));
            }
        }
        const FString Filter = Conditions.Num() == 1
            ? Conditions[0]
            : TEXT("or(") + FString::Join(Conditions, TEXT(",")) + TEXT(")");
        WSSvc->SetEventFilter(Filter);
    }

    TSet<int64> TilesToLoad;

    if (Zoom != LoadedZoom)
    {
        // Zoom changed — start fresh
        Factory->DestroyAllActors();
        TilesToLoad = NewTileKeys;
    }
    else
    {
        // Same zoom — only unload tiles leaving the set, load tiles entering it
        for (int64 Key : LoadedTileKeys.Difference(NewTileKeys))
            Factory->DestroyActorsForTile(Key);

        TilesToLoad = NewTileKeys.Difference(LoadedTileKeys);
    }

    LoadedTileKeys = NewTileKeys;
    LoadedZoom     = Zoom;

    UWorld *W = GetWorld();
    for (int64 TileKey : TilesToLoad)
    {
        // Fetch once per known geotile storage precision (see GeotileStorageZooms) — a tile
        // may hold entities from sources encoded at either zoom. SpawnTempUIActorForTile()
        // already dedups by ThingId, so a thing matched by more than one pass is harmless.
        for (int32 StorageZoom : GeotileStorageZooms)
        {
            int64 Lower, Upper;
            UDittoService::GetTileBoundsFromKey(TileKey, Zoom, Lower, Upper, StorageZoom);

            DittoSvc->GetThingsByGeotileBounds(Lower, Upper,
                [this, W, TileKey](const TArray<TSharedPtr<FJsonObject>>& Page)
                {
                    AsyncTask(ENamedThreads::GameThread, [this, W, TileKey, Page]()
                    {
                        if (!IsValid(this) || !IsValid(W)) return;
                        if (UGameInstance *GI2 = GetGameInstance())
                        {
                            if (UDT4MOBEntityFactory *F = GI2->GetSubsystem<UDT4MOBEntityFactory>())
                            {
                                for (const auto& Thing : Page)
                                    F->SpawnTempUIActorForTile(W, Thing, TileKey);
                            }
                        }
                    });
                },
                [TileKey, Zoom, StorageZoom]()
                {
                    UE_LOG(LogTemp, Log, TEXT("Tile %lld loaded at zoom %d (geotile precision %d)"), TileKey, Zoom, StorageZoom);
                });
        }
    }
}

TSet<int64> ADT4MOBGamemode::GetNeighborTileKeys(double Lat, double Lng, int32 Zoom) const
{
    const int64 TileCount = int64(1) << Zoom;
    int64 CX, CY;
    UDittoService::GetTileXY(Lat, Lng, Zoom, CX, CY);

    TSet<int64> Keys;
    for (int32 DX = -1; DX <= 1; ++DX)
    {
        for (int32 DY = -1; DY <= 1; ++DY)
        {
            const int64 TX = FMath::Clamp(CX + DX, int64(0), TileCount - 1);
            const int64 TY = FMath::Clamp(CY + DY, int64(0), TileCount - 1);
            Keys.Add(UDittoService::GetQuadkeyFromXY(TX, TY, Zoom));
        }
    }
    return Keys;
}

// ------------------------------------------------------------------ //
//  Legacy helper (kept for compatibility)
// ------------------------------------------------------------------ //

void ADT4MOBGamemode::OnCompletedGetAllThings(const TArray<TSharedPtr<FJsonValue>> &Things)
{
    int32 Counter = 0;
    UWorld *World = GetWorld();

    for (const TSharedPtr<FJsonValue> &ThingValue : Things)
    {
        TSharedPtr<FJsonValue> LocalVal = ThingValue;
        AsyncTask(ENamedThreads::GameThread, [World, LocalVal, this]()
                  {
            TSharedPtr<FJsonObject> ThingObject = LocalVal->AsObject();
            if (!ThingObject.IsValid()) return;

            FString Name = ThingObject->GetStringField(TEXT("thingId"));
            UE_LOG(LogTemp, Log, TEXT("Spawning Thing: %s"), *Name);

            if (UGameInstance* GI = GetGameInstance())
            {
                if (UDT4MOBEntityFactory* Factory = GI->GetSubsystem<UDT4MOBEntityFactory>())
                {
                    Factory->SpawnTempUIActor(World, ThingObject);
                }
            } });

        if (++Counter % 100 == 0)
        {
            UE_LOG(LogTemp, Log, TEXT("Processed %d things"), Counter);
        }
    }
}