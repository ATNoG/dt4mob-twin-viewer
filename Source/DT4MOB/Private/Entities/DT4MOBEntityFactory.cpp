// Fill out your copyright notice in the Description page of Project Settings.

/** @file DT4MOBEntityFactory.cpp
 *  @brief Implementation of UDT4MOBEntityFactory. All logic documentation is in the header.
 */
#include "Entities/DT4MOBEntityFactory.h"
#include "Entities/EntityTypeRegistrations.h"
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

UDT4MOBEntityFactory::UDT4MOBEntityFactory()
{
    DefaultExtension = NewObject<UEntityTypeExtension>(this, UEntityTypeExtension::StaticClass(), TEXT("DefaultEntityTypeExtension"));
    RegisterAllEntityTypes(*this);
}

void UDT4MOBEntityFactory::RegisterType(const FString& Key, UScriptStruct* Struct, const FString& DisplayName, bool bNoServerHandling,
                                         const FString& MeshPath, TSubclassOf<UEntityTypeExtension> ExtensionClass)
{
    ThingStructMap.Add(Key, Struct);
    FEntityTypeMeta Meta;
    Meta.TypeKey          = Key;
    Meta.DisplayName      = DisplayName;
    Meta.bNoServerHandling = bNoServerHandling;
    Meta.DefaultMeshPath  = MeshPath;
    TypeMetaMap.Add(Key, Meta);

    UEntityTypeExtension* Extension = DefaultExtension.Get();
    if (ExtensionClass)
    {
        // Object names can't contain UObject path-delimiter characters (':', '.', etc.),
        // which several type keys do (e.g. "fire:", ".instrument.") — strip anything
        // that isn't alphanumeric before using the key as part of the object name.
        FString SanitizedKey;
        for (TCHAR C : Key)
            if (FChar::IsAlnum(C)) SanitizedKey.AppendChar(C);

        const FName ObjName = MakeUniqueObjectName(this, ExtensionClass, FName(*(TEXT("Ext_") + SanitizedKey)));
        Extension = NewObject<UEntityTypeExtension>(this, ExtensionClass, ObjName);
    }
    TypeExtensionMap.Add(Key, Extension);
}

void UDT4MOBEntityFactory::RegisterMeshOverride(const FString& ThingId, const TArray<FString>& MeshPaths)
{
    ThingMeshOverrideMap.Add(ThingId, MeshPaths);
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

UEntityTypeExtension* UDT4MOBEntityFactory::GetExtensionForType(const FString& TypeKey) const
{
    if (const TObjectPtr<UEntityTypeExtension>* Found = TypeExtensionMap.Find(TypeKey))
        return *Found;
    return DefaultExtension;
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