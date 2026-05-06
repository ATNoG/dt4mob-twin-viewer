// Fill out your copyright notice in the Description page of Project Settings.

/** @file DT4MOBGamemode.cpp
 *  @brief Implementation of ADT4MOBGamemode. All logic documentation is in the header.
 */
#include "Gameplay/DT4MOBGamemode.h"
#include "Services/DittoService.h"
#include "Services/EntityUpdateDaemon.h"
#include "Json.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "Gameplay/UnifiedPawn/UnifiedPawn.h"
#include "CesiumGeoreference.h"
#include "Kismet/GameplayStatics.h"

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

    // ---- 1. Initial entity snapshot via DittoService ----------------------
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