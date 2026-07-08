// Fill out your copyright notice in the Description page of Project Settings.

/** @file DT4MOBEntityFactory.cpp
 *  @brief Implementation of UDT4MOBEntityFactory. All logic documentation is in the header.
 */
#include "Entities/DT4MOBEntityFactory.h"
#include "Json.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Services/CoordinatesConversionService.h"
#include "Services/ActorRegistryService.h"
#include "Engine/GameInstance.h"
#include "JsonObjectConverter.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "EntityStructs/MeteorologyStruct.h"
#include "EntityStructs/CarStruct.h"
#include "EntityStructs/BarrierStruct.h"
#include "EntityStructs/SignStruct.h"
#include "EntityStructs/TaludeStruct.h"
#include "EntityStructs/Equivia_AcessosServentiasStruct.h"
#include "EntityStructs/Equivia_DrenagemPontualStruct.h"
#include "EntityStructs/Equivia_IluminacaoStruct.h"
#include "EntityStructs/Equivia_IntegracaoPaisagisticaStruct.h"
#include "EntityStructs/Equivia_MarcosQuilometricosStruct.h"
#include "EntityStructs/Equivia_PavimentosStruct.h"
#include "EntityStructs/Equivia_SeccoesStruct.h"
#include "EntityStructs/Equivia_VedacoesStruct.h"
#include "EntityStructs/Equivia_TaludesStruct.h"
#include "EntityStructs/TollCameraStruct.h"
#include "EntityStructs/TollStruct.h"
#include "EntityStructs/GeoAssetStruct.h"
#include "EntityStructs/IgnitionPointStruct.h"
#include "EntityStructs/InfraestruturasPortugal_IluminacaoStruct.h"
#include "EntityStructs/InfraestruturasPortugal_SinalizacaoStruct.h"

UDT4MOBEntityFactory::UDT4MOBEntityFactory()
{
    auto Register = [&](const FString& Key, UScriptStruct* Struct, const FString& DisplayName, bool bNoServerHandling, const FString& MeshPath = FString())
    {
        ThingStructMap.Add(Key, Struct);
        FEntityTypeMeta Meta;
        Meta.TypeKey          = Key;
        Meta.DisplayName      = DisplayName;
        Meta.bNoServerHandling = bNoServerHandling;
        Meta.DefaultMeshPath  = MeshPath;
        TypeMetaMap.Add(Key, Meta);
    };

    Register("meteo",      FMeteorologyData::StaticStruct(), TEXT("Meteo Station"), true);
    Register("traci",      FCarData::StaticStruct(),         TEXT("Vehicle"),       true);
    // Register("barrier",    FBarrierData::StaticStruct(),     TEXT("Barrier"),       true);
    // Register("sign",       FSignData::StaticStruct(),        TEXT("Road Sign"),     true,  TEXT("/Game/Models/Temp/sign/StaticMeshes/sign.sign"));
    // Register("muro-talude",FTaludeData::StaticStruct(),      TEXT("Slope"),         true);
    // Register("tolls:camera", FTollCameraData::StaticStruct(),    TEXT("Toll Camera"),  true);
    // Register("tolls:toll",   FTollData::StaticStruct(),          TEXT("Toll Plaza"),   true);

    // Equivia entities
    // Register("equivia:AcessosServentias",    FAcessosServentiasData::StaticStruct(),     TEXT("Access/Serventia"),       true);
    // Register("equivia:DrenagemPontual",      FDrenagemPontualData::StaticStruct(),       TEXT("Drainage Point"),         true);
    // Register("equivia:Iluminacao",           FIluminacaoData::StaticStruct(),            TEXT("Lighting"),               true, TEXT("/Game/Models/Temp/Streetlight/StaticMeshes/Streetlight.Streetlight"));
    Register("InfraestruturasPortugal:iluminacao",   FInfPtIluminacaoData::StaticStruct(),    TEXT("IP Lighting"),  true, TEXT("/Game/Models/Temp/Streetlight/StaticMeshes/Streetlight.Streetlight"));
    Register("InfraestruturasPortugal:sinalizacao",  FInfPtSinalizacaoData::StaticStruct(),   TEXT("IP Sign"),      true, TEXT("/Game/Models/Temp/signempty/StaticMeshes/signempty.signempty"));
    // Register("equivia:IntegracaoPaisagistica", FIntegracaoPaisagisticaData::StaticStruct(), TEXT("Landscape Integration"), true);
    // Register("equivia:MarcosQuilometricos",  FMarcosQuilometricosData::StaticStruct(),   TEXT("Km Marker"),              true);
    // Register("equivia:Pavimentos",           FPavimentosData::StaticStruct(),            TEXT("Pavement"),               true);
    // Register("equivia:Seccoes",              FSeccoesData::StaticStruct(),               TEXT("Road Section"),           true);
    // Register("equivia:Taludes",              FEquiviaTaludesData::StaticStruct(),        TEXT("Equivia Slope"),          true);
    // Register("equivia:Vedacoes",             FVedacoesData::StaticStruct(),              TEXT("Fencing"),                true);

    // // Geo-asset entities — ".instrument." is more specific than "geo-asset" and wins via longest-match
    Register(".instrument.", FGeoInstrumentData::StaticStruct(), TEXT("Geo Instrument"), true);
    Register("geo-asset",    FGeoAssetData::StaticStruct(),      TEXT("Geo Asset"),      true);
    Register("fire:",        FIgnitionPointData::StaticStruct(), TEXT("Ignition Point"), false);

    ThingMeshOverrideMap.Add(
        TEXT("geo-asset:brisa:e27a9388-84c5-4081-8c85-b7e8abc2b09a"),
        {
            TEXT("/Game/Models/Talude/r59zdz3.r59zdz3"),
            TEXT("/Game/Models/Talude/rs65kp1.rs65kp1"),
            TEXT("/Game/Models/Talude/testtalude.testtalude"),
        }
    );
}

ATempUIActor *UDT4MOBEntityFactory::SpawnTempUIActor(UWorld *World, TSharedPtr<FJsonObject> ThingData)
{
    if (!IsValid(World))
    {
        // UE_LOG(LogTemp, Error, TEXT("UDT4MOBEntityFactory::SpawnTempUIActor: World is null"));
        return nullptr;
    }

    if (!ThingData.IsValid())
    {
        // UE_LOG(LogTemp, Warning, TEXT("UDT4MOBEntityFactory::SpawnTempUIActor: ThingData is null/invalid"));
        return nullptr;
    }

    // A live actor for this ThingId may already exist (e.g. it survived a tile refresh as a
    // protected in-view/windowed orphan, or it's simply already spawned). Refresh it in place
    // instead of spawning a duplicate — checked here so every call site (tile refresh, initial
    // load, on-demand WS spawn) gets the same guarantee.
    const FString IncomingThingId = ThingData->GetStringField(TEXT("thingId"));
    if (!IncomingThingId.IsEmpty())
    {
        if (UGameInstance *GI = GetGameInstance())
        {
            if (UActorRegistryService *Registry = GI->GetSubsystem<UActorRegistryService>())
            {
                if (ATempUIActor *Existing = Registry->FindActor(IncomingThingId))
                {
                    if (UScriptStruct *ExistingStructType = GetStructForThing(ThingData))
                        Existing->Initialize(ExistingStructType, ThingData);
                    return Existing;
                }
            }
        }
    }

    UScriptStruct *StructType = GetStructForThing(ThingData);
    if (!StructType)
    {
        LogUnknownThing(ThingData);
        return nullptr;
    }

    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;

    ATempUIActor *NewActor = World->SpawnActor<ATempUIActor>(ATempUIActor::StaticClass(), Location, Rotation);
    if (!NewActor)
    {
        // UE_LOG(LogTemp, Error, TEXT("Failed to spawn TempUIActor for ThingId: %s"), *ThingData->GetStringField(TEXT("thingId")));
        return nullptr;
    }

    NewActor->Initialize(StructType, ThingData);

    // Apply the per-type default mesh if one is registered.
    FString ThingIdForMesh = ThingData->GetStringField(TEXT("thingId"));
    FString TypeKey = GetTypeKeyForThingId(ThingIdForMesh);
    if (const FEntityTypeMeta* Meta = TypeMetaMap.Find(TypeKey))
    {
        if (!Meta->DefaultMeshPath.IsEmpty())
        {
            if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Meta->DefaultMeshPath))
                NewActor->StaticMeshComponent->SetStaticMesh(Mesh);
        }
    }

    // Apply per-thingId mesh overrides — find existing level actors rather than creating new components.
    if (const TArray<FString>* OverridePaths = ThingMeshOverrideMap.Find(ThingIdForMesh))
    {
        for (const FString& MeshPath : *OverridePaths)
        {
            const FString LayerName = FPaths::GetBaseFilename(MeshPath);

            // Search the level for an existing actor whose label matches the layer name.
            AStaticMeshActor* LevelActor = nullptr;
            for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
            {
                if (It->GetActorNameOrLabel() == LayerName)
                {
                    LevelActor = *It;
                    break;
                }
            }

            if (LevelActor && LevelActor->GetStaticMeshComponent())
            {
                NewActor->MeshLayers.Add(LayerName, LevelActor->GetStaticMeshComponent());
                NewActor->OnMeshLayersChanged.Broadcast();
            }
            else if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath))
            {
                NewActor->AddOrReplaceMeshLayer(LayerName, Mesh);
            }
        }
        NewActor->StaticMeshComponent->SetVisibility(false);
    }

    SpawnedActors.Add(NewActor);

    return NewActor;
}

UScriptStruct *UDT4MOBEntityFactory::GetStructForThing(TSharedPtr<FJsonObject> ThingData)
{
    if (!ThingData.IsValid())
    {
        return nullptr;
    }

    FString ThingId = ThingData->GetStringField(TEXT("thingId"));

    // Pick the longest matching key so more-specific entries beat broader ones
    // (e.g. ".instrument." beats "geo-asset" for instrument thingIds).
    UScriptStruct *Best = nullptr;
    int32 BestLen = -1;
    for (const auto &Pair : ThingStructMap)
    {
        if (ThingId.Contains(Pair.Key) && Pair.Key.Len() > BestLen)
        {
            Best = Pair.Value;
            BestLen = Pair.Key.Len();
        }
    }

    return Best;
}

bool UDT4MOBEntityFactory::CanHandleThingId(const FString &ThingId) const
{
    for (const auto &Pair : ThingStructMap)
    {
        if (ThingId.Contains(Pair.Key))
            return true;
    }
    return false;
}

FString UDT4MOBEntityFactory::GetTypeKeyForThingId(const FString& ThingId) const
{
    FString BestKey;
    int32 BestLen = -1;
    for (const auto& Pair : ThingStructMap)
    {
        if (ThingId.Contains(Pair.Key) && Pair.Key.Len() > BestLen)
        {
            BestKey = Pair.Key;
            BestLen = Pair.Key.Len();
        }
    }
    return BestKey;
}

bool UDT4MOBEntityFactory::ShouldProtectActor(const ATempUIActor* Actor)
{
    return IsValid(Actor) && (Actor->IsInCameraView() || Actor->HasOpenWindow());
}

void UDT4MOBEntityFactory::DestroyAllActors()
{
    TArray<TWeakObjectPtr<ATempUIActor>> Survivors;
    Survivors.Reserve(SpawnedActors.Num());

    for (TWeakObjectPtr<ATempUIActor> &ActorPtr : SpawnedActors)
    {
        if (!ActorPtr.IsValid())
            continue;

        if (ShouldProtectActor(ActorPtr.Get()))
        {
            OrphanedActors.AddUnique(ActorPtr);
            Survivors.Add(ActorPtr);
        }
        else
        {
            ActorPtr->Destroy();
        }
    }

    SpawnedActors = MoveTemp(Survivors);
    TileActorMap.Empty();
}

ATempUIActor* UDT4MOBEntityFactory::SpawnTempUIActorForTile(UWorld* World, TSharedPtr<FJsonObject> ThingData, int64 TileKey)
{
    // SpawnTempUIActor() itself now dedups by ThingId and returns the existing live actor
    // (e.g. a protected orphan re-entering this tile's range) instead of creating a duplicate.
    ATempUIActor* Actor = SpawnTempUIActor(World, ThingData);
    if (!Actor)
        return nullptr;

    OrphanedActors.RemoveSingleSwap(TWeakObjectPtr<ATempUIActor>(Actor));
    TileActorMap.FindOrAdd(TileKey).AddUnique(Actor);
    return Actor;
}

void UDT4MOBEntityFactory::DestroyActorsForTile(int64 TileKey)
{
    TArray<TWeakObjectPtr<ATempUIActor>>* Actors = TileActorMap.Find(TileKey);
    if (!Actors) return;

    for (TWeakObjectPtr<ATempUIActor>& ActorPtr : *Actors)
    {
        if (!ActorPtr.IsValid())
            continue;

        if (ShouldProtectActor(ActorPtr.Get()))
        {
            OrphanedActors.AddUnique(ActorPtr);
            // Stays in SpawnedActors — it's still alive, just tile-less until re-homed or swept.
        }
        else
        {
            SpawnedActors.RemoveSingleSwap(ActorPtr);
            ActorPtr->Destroy();
        }
    }
    TileActorMap.Remove(TileKey);
}

void UDT4MOBEntityFactory::SweepOrphanedActors()
{
    for (int32 i = OrphanedActors.Num() - 1; i >= 0; --i)
    {
        TWeakObjectPtr<ATempUIActor>& Ptr = OrphanedActors[i];
        if (!Ptr.IsValid())
        {
            OrphanedActors.RemoveAtSwap(i);
            continue;
        }

        if (!ShouldProtectActor(Ptr.Get()))
        {
            ATempUIActor* Actor = Ptr.Get();
            SpawnedActors.RemoveSingleSwap(Ptr);
            OrphanedActors.RemoveAtSwap(i);
            Actor->Destroy();
        }
    }
}

bool UDT4MOBEntityFactory::IsTileLoaded(int64 TileKey) const
{
    return TileActorMap.Contains(TileKey);
}

void UDT4MOBEntityFactory::LogUnknownThing(TSharedPtr<FJsonObject> ThingData)
{
    // log to file
    FString ThingDataString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ThingDataString);
    if (FJsonSerializer::Serialize(ThingData.ToSharedRef(), Writer))
    {
        const FString FilePath = FPaths::ProjectDir() + TEXT("logs/");
        IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

        if (!PlatformFile.DirectoryExists(*FilePath))
        {
            PlatformFile.CreateDirectoryTree(*FilePath);
        }

        const FString ThingId = ThingData->GetStringField(TEXT("thingId"));
        const FString SafeThingId = ThingId.IsEmpty() ? TEXT("UnknownThing") : ThingId;
        const FString ActThingId = SafeThingId.Replace(TEXT(":"), TEXT("_"));

        const FString FullFilePath = FilePath + ActThingId + TEXT(".json");
        // check if file already exists, if so skip writing
        UE_LOG(LogTemp, Log, TEXT("DittoFactory: serialized [%s] → %d chars"), *ActThingId, ThingDataString.Len());

        if (!PlatformFile.FileExists(*FullFilePath))
        {
            UE_LOG(LogTemp, Log, TEXT("DittoFactory: logging unknown thing to %s"), *FullFilePath);
            if (FFileHelper::SaveStringToFile(ThingDataString, *FullFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
            {
                UE_LOG(LogTemp, Log, TEXT("DittoFactory: saved %s"), *FullFilePath);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("DittoFactory: failed to save %s"), *FullFilePath);
            }
        }
        else
        {
            UE_LOG(LogTemp, Verbose, TEXT("DittoFactory: skipping existing log for %s"), *FullFilePath);
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("DittoFactory: JSON serialize failed for unknown thing"));
    }
}