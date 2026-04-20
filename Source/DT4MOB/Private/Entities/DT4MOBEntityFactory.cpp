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

UDT4MOBEntityFactory::UDT4MOBEntityFactory()
{
    ThingStructMap.Add("meteo", FMeteorologyData::StaticStruct());
    ThingStructMap.Add("traci", FCarData::StaticStruct());
    // ThingStructMap.Add("barrier", FBarrierData::StaticStruct());
    // ThingStructMap.Add("sign", FSignData::StaticStruct());
    // ThingStructMap.Add("muro-talude", FTaludeData::StaticStruct());
    // ThingStructMap.Add("tolls:camera", FTollCameraData::StaticStruct());
    // ThingStructMap.Add("tolls:toll", FTollData::StaticStruct());

    // Equivia entities
    ThingStructMap.Add("equivia:AcessosServentias", FAcessosServentiasData::StaticStruct());
    ThingStructMap.Add("equivia:DrenagemPontual", FDrenagemPontualData::StaticStruct());
    ThingStructMap.Add("equivia:Iluminacao", FIluminacaoData::StaticStruct());
    ThingStructMap.Add("equivia:IntegracaoPaisagistica", FIntegracaoPaisagisticaData::StaticStruct());
    ThingStructMap.Add("equivia:MarcosQuilometricos", FMarcosQuilometricosData::StaticStruct());
    ThingStructMap.Add("equivia:Pavimentos", FPavimentosData::StaticStruct());
    ThingStructMap.Add("equivia:Seccoes", FSeccoesData::StaticStruct());
    ThingStructMap.Add("equivia:Taludes", FEquiviaTaludesData::StaticStruct());
    ThingStructMap.Add("equivia:Vedacoes", FVedacoesData::StaticStruct());
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

    return NewActor;
}

UScriptStruct *UDT4MOBEntityFactory::GetStructForThing(TSharedPtr<FJsonObject> ThingData)
{
    if (!ThingData.IsValid())
    {
        return nullptr;
    }

    FString ThingId = ThingData->GetStringField(TEXT("thingId"));

    for (const auto &Pair : ThingStructMap)
    {
        if (ThingId.Contains(Pair.Key))
        {
            return Pair.Value;
        }
    }

    return nullptr;
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