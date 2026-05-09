// Fill out your copyright notice in the Description page of Project Settings.

/** @file DT4MOBGamemode.cpp
 *  @brief Implementation of ADT4MOBGamemode. All logic documentation is in the header.
 */
#include "Gameplay/DT4MOBGamemode.h"
#include "Services/DittoService.h"
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

    // ---- 2. Initial entity snapshot via DittoService ----------------------
    if (UDittoService *DittoService = GameInstance->GetSubsystem<UDittoService>())
    {
        DittoService->GetAllThings(
            [this](const TArray<TSharedPtr<FJsonObject>> &Page)
            {
                UWorld *W = GetWorld();
                if (!W)
                    return;

                AsyncTask(ENamedThreads::GameThread, [this, W, Page]()
                          {
                    if (UGameInstance* GI = GetGameInstance())
                    {
                        if (UDT4MOBEntityFactory* Factory = GI->GetSubsystem<UDT4MOBEntityFactory>())
                        {
                            for (const auto& Thing : Page)
                            {
                                Factory->SpawnTempUIActor(W, Thing);
                            }
                        }
                    } });
            },
            []()
            {
                UE_LOG(LogTemp, Log, TEXT("ADT4MOBGamemode: finished loading all things"));
            });
    }
}

void ADT4MOBGamemode::HandleUnhandledThingMessage(const FString &ThingId, const FString &Path, const FString &ValueJson)
{
    UGameInstance *GI = GetGameInstance();
    if (!GI) return;

    UDT4MOBEntityFactory *Factory = GI->GetSubsystem<UDT4MOBEntityFactory>();
    if (!Factory || !Factory->CanHandleThingId(ThingId)) return;

    // Deduplicate: one fetch/spawn per thingId at a time
    if (PendingSpawnThingIds.Contains(ThingId)) return;

    UEntityUpdateDaemon *D = GI->GetSubsystem<UEntityUpdateDaemon>();
    UWorld *W = GetWorld();
    if (!D || !W) return;

    // Fast path: "created" events carry path="/" with the full thing JSON.
    // Spawn directly without an HTTP round-trip, then replay this message so
    // the actor's position/dimensions get applied immediately.
    if (Path == TEXT("/") && !ValueJson.IsEmpty() && ValueJson != TEXT("{}"))
    {
        TSharedPtr<FJsonObject> ThingJson;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ValueJson);
        if (FJsonSerializer::Deserialize(Reader, ThingJson) && ThingJson.IsValid())
        {
            ATempUIActor *NewActor = Factory->SpawnTempUIActor(W, ThingJson);
            if (NewActor)
            {
                UE_LOG(LogTemp, Log, TEXT("ADT4MOBGamemode: spawned [%s] from WS created event"), *ThingId);
                // No replay needed: Initialize() already processed the full thing JSON.
            }
            return;
        }
    }

    // Slow path: "modified" arrived for a thing we haven't seen yet.
    // Fetch the full thing from Ditto, spawn, then replay the triggering update.
    PendingSpawnThingIds.Add(ThingId);

    UDittoService *DittoSvc = GI->GetSubsystem<UDittoService>();
    if (!DittoSvc)
    {
        PendingSpawnThingIds.Remove(ThingId);
        return;
    }

    FString CapturedThingId   = ThingId;
    FString CapturedPath      = Path;
    FString CapturedValueJson = ValueJson;

    DittoSvc->GetThingById(ThingId, [this, CapturedThingId, CapturedPath, CapturedValueJson](TSharedPtr<FJsonObject> ThingJson)
    {
        AsyncTask(ENamedThreads::GameThread, [this, CapturedThingId, CapturedPath, CapturedValueJson, ThingJson]()
        {
            PendingSpawnThingIds.Remove(CapturedThingId);
            if (!IsValid(this)) return;

            UGameInstance *GI2 = GetGameInstance();
            if (!GI2) return;

            UDT4MOBEntityFactory *F = GI2->GetSubsystem<UDT4MOBEntityFactory>();
            UEntityUpdateDaemon  *D2 = GI2->GetSubsystem<UEntityUpdateDaemon>();
            UWorld *W2 = GetWorld();
            if (!F || !D2 || !W2) return;

            if (!ThingJson.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("ADT4MOBGamemode: on-demand fetch failed for [%s]"), *CapturedThingId);
                return;
            }

            ATempUIActor *NewActor = F->SpawnTempUIActor(W2, ThingJson);
            if (!NewActor)
            {
                UE_LOG(LogTemp, Warning, TEXT("ADT4MOBGamemode: on-demand spawn failed for [%s]"), *CapturedThingId);
                return;
            }

            UE_LOG(LogTemp, Log, TEXT("ADT4MOBGamemode: on-demand spawned [%s] (via HTTP fetch)"), *CapturedThingId);
            D2->InjectUpdate(CapturedThingId, CapturedPath, CapturedValueJson);
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
    // Super::Tick(DeltaSeconds);
    // CheckAndRefreshTiles(DeltaSeconds);
}

void ADT4MOBGamemode::CheckAndRefreshTiles(float DeltaSeconds)
{
    APlayerController *PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (!PC) return;

    AUnifiedPawn *Pawn = Cast<AUnifiedPawn>(PC->GetPawn());
    if (!Pawn) return;

    ACesiumGeoreference *Georeference = ACesiumGeoreference::GetDefaultGeoreferenceForActor(Pawn);
    if (!Georeference) return;

    // Convert pawn world position to (Longitude, Latitude, AltitudeMeters)
    const FVector LLH = Georeference->TransformUnrealPositionToLongitudeLatitudeHeight(Pawn->GetActorLocation());
    const double Lng = LLH.X;
    const double Lat = LLH.Y;

    // Camera altitude = pawn ellipsoid altitude + spring arm length (cm → m)
    const double CameraAltMeters = LLH.Z + Pawn->GetTargetArmLength() / 100.0;

    const int32 Zoom = UDittoService::AltitudeToZoomLevel(CameraAltMeters);

    // Below the minimum zoom threshold, tile filtering is too coarse to be useful
    if (Zoom < MinZoomForTileFiltering) return;

    int64 Lower, Upper;
    UDittoService::GetTileBounds(Lat, Lng, Zoom, Lower, Upper);

    // Already queried this exact tile — nothing to do
    if (Lower == LastTileLower && Upper == LastTileUpper) return;

    if (Lower == PendingTileLower && Upper == PendingTileUpper && bPendingTileRefresh)
    {
        // Still in the same pending tile — count down the debounce
        TileRefreshTimer -= DeltaSeconds;
        if (TileRefreshTimer <= 0.f)
        {
            bPendingTileRefresh = false;
            LastTileLower = PendingTileLower;
            LastTileUpper = PendingTileUpper;
            LastTileZoom  = PendingZoom;
            DoTileRefresh(PendingLat, PendingLng, PendingZoom);
        }
    }
    else
    {
        // Moved into a new tile — restart the debounce
        PendingLat       = Lat;
        PendingLng       = Lng;
        PendingZoom      = Zoom;
        PendingTileLower = Lower;
        PendingTileUpper = Upper;
        bPendingTileRefresh = true;
        TileRefreshTimer = TileRefreshDelay;
    }
}

void ADT4MOBGamemode::DoTileRefresh(double Lat, double Lng, int32 TileZoom)
{
    UGameInstance *GI = GetGameInstance();
    if (!GI) return;

    UDT4MOBEntityFactory *Factory = GI->GetSubsystem<UDT4MOBEntityFactory>();
    UDittoService        *DittoSvc = GI->GetSubsystem<UDittoService>();
    if (!Factory || !DittoSvc) return;

    Factory->DestroyAllActors();

    UWorld *W = GetWorld();
    DittoSvc->GetThingsByGeotile(
        Lat, Lng, TileZoom,
        [this, W](const TArray<TSharedPtr<FJsonObject>> &Page)
        {
            AsyncTask(ENamedThreads::GameThread, [this, W, Page]()
            {
                if (!IsValid(this) || !IsValid(W)) return;
                if (UGameInstance *GI2 = GetGameInstance())
                {
                    if (UDT4MOBEntityFactory *F = GI2->GetSubsystem<UDT4MOBEntityFactory>())
                    {
                        for (const auto &Thing : Page)
                            F->SpawnTempUIActor(W, Thing);
                    }
                }
            });
        },
        [TileZoom](){ UE_LOG(LogTemp, Log, TEXT("Tile refresh complete at zoom %d"), TileZoom); }
    );
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