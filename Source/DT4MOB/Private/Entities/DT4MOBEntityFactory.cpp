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
#include "JsonObjectConverter.h"
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

UDT4MOBEntityFactory::UDT4MOBEntityFactory()
{
    auto Register = [&](const FString& Key, UScriptStruct* Struct, const FString& DisplayName, bool bNoServerHandling)
    {
        ThingStructMap.Add(Key, Struct);
        TypeMetaMap.Add(Key, FEntityTypeMeta{Key, DisplayName, bNoServerHandling});
    };

    Register("meteo", FMeteorologyData::StaticStruct(), TEXT("Meteo Station"), true);
    Register("traci",        FCarData::StaticStruct(),           TEXT("Vehicle"),      true);
    Register("barrier",   FBarrierData::StaticStruct(),       TEXT("Barrier"),      true);
    Register("sign",      FSignData::StaticStruct(),          TEXT("Road Sign"),    true);
    Register("muro-talude", FTaludeData::StaticStruct(),      TEXT("Slope"),        true);
    // Register("tolls:camera", FTollCameraData::StaticStruct(),    TEXT("Toll Camera"),  true);
    // Register("tolls:toll",   FTollData::StaticStruct(),          TEXT("Toll Plaza"),   true);

    // Equivia entities
    // Register("equivia:AcessosServentias",    FAcessosServentiasData::StaticStruct(),     TEXT("Access/Serventia"),       true);
    // Register("equivia:DrenagemPontual",      FDrenagemPontualData::StaticStruct(),       TEXT("Drainage Point"),         true);
    // Register("equivia:Iluminacao",           FIluminacaoData::StaticStruct(),            TEXT("Lighting"),               true);
    // Register("equivia:IntegracaoPaisagistica", FIntegracaoPaisagisticaData::StaticStruct(), TEXT("Landscape Integration"), true);
    // Register("equivia:MarcosQuilometricos",  FMarcosQuilometricosData::StaticStruct(),   TEXT("Km Marker"),              true);
    // Register("equivia:Pavimentos",           FPavimentosData::StaticStruct(),            TEXT("Pavement"),               true);
    // Register("equivia:Seccoes",              FSeccoesData::StaticStruct(),               TEXT("Road Section"),           true);
    // Register("equivia:Taludes",              FEquiviaTaludesData::StaticStruct(),        TEXT("Equivia Slope"),          true);
    // Register("equivia:Vedacoes",             FVedacoesData::StaticStruct(),              TEXT("Fencing"),                true);

    // Geo-asset entities — ".instrument." is more specific than "geo-asset" and wins via longest-match
    Register(".instrument.", FGeoInstrumentData::StaticStruct(), TEXT("Geo Instrument"), true);
    Register("geo-asset",    FGeoAssetData::StaticStruct(),      TEXT("Geo Asset"),      true);
    Register("fire:",        FIgnitionPointData::StaticStruct(), TEXT("Ignition Point"), false);
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

void UDT4MOBEntityFactory::DestroyAllActors()
{
    for (TWeakObjectPtr<ATempUIActor> &ActorPtr : SpawnedActors)
    {
        if (ActorPtr.IsValid())
        {
            ActorPtr->Destroy();
        }
    }
    SpawnedActors.Empty();
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
        FString Left, Right;
        SafeThingId.Split(TEXT(":"), &Left, &Right);
        const FString ActThingId = Left + "_" + Right;

        const FString FullFilePath = FilePath + ActThingId + TEXT(".json");
        // check if file already exists, if so skip writing
        if (!PlatformFile.FileExists(*FullFilePath))
        {
            // UE_LOG(LogTemp, Log, TEXT("Logging unknown ThingData to file: %s"), *FullFilePath);
            if (FFileHelper::SaveStringToFile(ThingDataString, *FullFilePath))
            {
                // UE_LOG(LogTemp, Log, TEXT("Saved ThingData to file: %s"), *FullFilePath);
            }
            else
            {
                // UE_LOG(LogTemp, Error, TEXT("Failed to save ThingData to file: %s"), *FullFilePath);
            }
        }
        else
        {
            // UE_LOG(LogTemp, Log, TEXT("ThingData file already exists, skipping: %s"), *FullFilePath);
        }
    }
    else
    {
        // UE_LOG(LogTemp, Error, TEXT("Failed to serialize ThingData for logging"));
    }
}