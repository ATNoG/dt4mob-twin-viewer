// Fill out your copyright notice in the Description page of Project Settings.

/** @file EntityTypeRegistrations.cpp
 *  @brief The full Ditto entity-type registration table. Add a new type here.
 */
#include "Entities/EntityTypeRegistrations.h"
#include "Entities/DT4MOBEntityFactory.h"

#include "EntityStructs/MeteorologyStruct.h"
#include "EntityStructs/CarStruct.h"
#include "EntityStructs/BarrierStruct.h"
#include "EntityStructs/TaludeStruct.h"
#include "EntityStructs/TollCameraStruct.h"
#include "EntityStructs/TollStruct.h"
#include "EntityStructs/GeoAssetStruct.h"
#include "EntityStructs/IgnitionPointStruct.h"
#include "EntityStructs/InfraestruturasPortugal_IluminacaoStruct.h"
#include "EntityStructs/InfraestruturasPortugal_SinalizacaoStruct.h"
#include "EntityStructs/InfraestruturasPortugal_MarcasPontosStruct.h"
#include "EntityStructs/InfraestruturasPortugal_MarcasLinhasStruct.h"
#include "EntityStructs/InfraestruturasPortugal_MarcasPoligonosStruct.h"
#include "EntityStructs/InfraestruturasPortugal_PavimentosStruct.h"
#include "EntityStructs/InfraestruturasPortugal_MarcosQuilometricosStruct.h"
#include "EntityStructs/InfraestruturasPortugal_DrenagemLinearStruct.h"
#include "EntityStructs/InfraestruturasPortugal_ObrasContencaoStruct.h"
#include "EntityStructs/InfraestruturasPortugal_ObrasArteStruct.h"
#include "EntityStructs/InfraestruturasPortugal_TelematicaCcvStruct.h"
#include "EntityStructs/InfraestruturasPortugal_TelematicaVideovigilanciaStruct.h"
#include "EntityStructs/InfraestruturasPortugal_TelematicaPmvStruct.h"
#include "EntityStructs/InfraestruturasPortugal_TelematicaSosStruct.h"
#include "EntityStructs/InfraestruturasPortugal_PorticosStruct.h"
#include "EntityStructs/InfraestruturasPortugal_CoordenadasProjetoAprovadasStruct.h"
#include "EntityStructs/InfraestruturasPortugal_InfoRouteLnStruct.h"

#include "EntityDependencies/Vehicle/VehicleExtension.h"
#include "EntityDependencies/Meteo/MeteoExtension.h"
#include "EntityDependencies/Barrier/BarrierExtension.h"
#include "EntityDependencies/Talude/TaludeExtension.h"
#include "EntityDependencies/TollCamera/TollCameraExtension.h"
#include "EntityDependencies/Toll/TollExtension.h"
#include "EntityDependencies/Sinalizacao/SinalizacaoExtension.h"
#include "EntityDependencies/GeoInstrument/GeoInstrumentExtension.h"
#include "EntityDependencies/GeoAsset/GeoAssetExtension.h"
#include "EntityDependencies/IgnitionPoint/IgnitionPointExtension.h"
#include "EntityDependencies/Iluminacao/IluminacaoExtension.h"
#include "EntityDependencies/MarcasPontos/MarcasPontosExtension.h"
#include "EntityDependencies/MarcasLinhas/MarcasLinhasExtension.h"
#include "EntityDependencies/MarcasPoligonos/MarcasPoligonosExtension.h"
#include "EntityDependencies/Pavimentos/PavimentosExtension.h"
#include "EntityDependencies/MarcosQuilometricos/MarcosQuilometricosExtension.h"
#include "EntityDependencies/DrenagemLinear/DrenagemLinearExtension.h"
#include "EntityDependencies/ObrasContencao/ObrasContencaoExtension.h"
#include "EntityDependencies/ObrasArte/ObrasArteExtension.h"
#include "EntityDependencies/TelematicaCcv/TelematicaCcvExtension.h"
#include "EntityDependencies/TelematicaVideovigilancia/TelematicaVideovigilanciaExtension.h"
#include "EntityDependencies/TelematicaPmv/TelematicaPmvExtension.h"
#include "EntityDependencies/TelematicaSos/TelematicaSosExtension.h"
#include "EntityDependencies/Porticos/PorticosExtension.h"
#include "EntityDependencies/CoordenadasProjetoAprovadas/CoordenadasProjetoAprovadasExtension.h"
#include "EntityDependencies/InfoRouteLn/InfoRouteLnExtension.h"

void RegisterAllEntityTypes(UDT4MOBEntityFactory& Factory)
{
    Factory.RegisterType("meteo",      FMeteorologyData::StaticStruct(), TEXT("Meteo Station"), true, FString(), UMeteoExtension::StaticClass());
    Factory.RegisterType("traci",      FCarData::StaticStruct(),         TEXT("Vehicle"),       true, FString(), UVehicleExtension::StaticClass());
    Factory.RegisterType("barrier",    FBarrierData::StaticStruct(),     TEXT("Barrier"),       true, FString(), UBarrierExtension::StaticClass());
    Factory.RegisterType("muro-talude",FTaludeData::StaticStruct(),      TEXT("Slope"),         true, FString(), UTaludeExtension::StaticClass());
    Factory.RegisterType("tolls:camera", FTollCameraData::StaticStruct(),    TEXT("Toll Camera"),  true, FString(), UTollCameraExtension::StaticClass());
    Factory.RegisterType("tolls:toll",   FTollData::StaticStruct(),          TEXT("Toll Plaza"),   true, FString(), UTollExtension::StaticClass());
    Factory.RegisterType("iluminacao",   FInfPtIluminacaoData::StaticStruct(),    TEXT("IP Lighting"),  true, FString(), UIluminacaoExtension::StaticClass());
    Factory.RegisterType("sinalizacao",  FInfPtSinalizacaoData::StaticStruct(),   TEXT("IP Sign"),      true, FString(), USinalizacaoExtension::StaticClass());
    Factory.RegisterType("marcas-pontos",    FInfPtMarcasPontosData::StaticStruct(),    TEXT("Road Marking (Point)"),   true, FString(), UMarcasPontosExtension::StaticClass());
    Factory.RegisterType("marcas-linhas",    FInfPtMarcasLinhasData::StaticStruct(),    TEXT("Road Marking (Line)"),    true, FString(), UMarcasLinhasExtension::StaticClass());
    Factory.RegisterType("marcas-poligonos", FInfPtMarcasPoligonosData::StaticStruct(), TEXT("Road Marking (Polygon)"), true, FString(), UMarcasPoligonosExtension::StaticClass());
    Factory.RegisterType("pavimentos",       FInfPtPavimentosData::StaticStruct(),      TEXT("Pavement"),               true, FString(), UPavimentosExtension::StaticClass());
    Factory.RegisterType("marcos-quilometricos", FInfPtMarcosQuilometricosData::StaticStruct(), TEXT("Km Marker"),      true, FString(), UMarcosQuilometricosExtension::StaticClass());
    Factory.RegisterType("drenagem-linear",  FInfPtDrenagemLinearData::StaticStruct(),  TEXT("Linear Drainage"),        true, FString(), UDrenagemLinearExtension::StaticClass());
    Factory.RegisterType("obras-contencao",  FInfPtObrasContencaoData::StaticStruct(),  TEXT("Retaining Structure"),    true, FString(), UObrasContencaoExtension::StaticClass());
    Factory.RegisterType("obras-arte",       FInfPtObrasArteData::StaticStruct(),       TEXT("Structural Work"),        true, FString(), UObrasArteExtension::StaticClass());
    Factory.RegisterType("telematica-ccv",   FInfPtTelematicaCcvData::StaticStruct(),   TEXT("CCV Telematics"),         true, FString(), UTelematicaCcvExtension::StaticClass());
    Factory.RegisterType("telematica-sistemas-videovigilancia", FInfPtTelematicaVideovigilanciaData::StaticStruct(), TEXT("Video Surveillance"), true, FString(), UTelematicaVideovigilanciaExtension::StaticClass());
    Factory.RegisterType("telematica-pmv",   FInfPtTelematicaPmvData::StaticStruct(),   TEXT("Variable Message Panel"), true, FString(), UTelematicaPmvExtension::StaticClass());
    Factory.RegisterType("telematica-sos",   FInfPtTelematicaSosData::StaticStruct(),   TEXT("SOS Post"),               true, FString(), UTelematicaSosExtension::StaticClass());
    Factory.RegisterType("porticos",         FInfPtPorticosData::StaticStruct(),        TEXT("Gantry"),                 true, FString(), UPorticosExtension::StaticClass());
    Factory.RegisterType("coordenadas-de-projeto-aprovadas-ip", FInfPtCoordenadasProjetoAprovadasData::StaticStruct(), TEXT("Approved Project Coordinate"), true, FString(), UCoordenadasProjetoAprovadasExtension::StaticClass());
    Factory.RegisterType("info-route-ln",    FInfPtInfoRouteLnData::StaticStruct(),     TEXT("Route Info (Line)"),      true, FString(), UInfoRouteLnExtension::StaticClass());

    // Geo-asset entities — ".instrument." is more specific than "geo-asset" and wins via longest-match
    Factory.RegisterType(".instrument.", FGeoInstrumentData::StaticStruct(), TEXT("Geo Instrument"), true, FString(), UGeoInstrumentExtension::StaticClass());
    Factory.RegisterType("geo-asset",    FGeoAssetData::StaticStruct(),      TEXT("Geo Asset"),      true, FString(), UGeoAssetExtension::StaticClass());
    Factory.RegisterType("fire:",        FIgnitionPointData::StaticStruct(), TEXT("Ignition Point"), false, FString(), UIgnitionPointExtension::StaticClass());

    // Still missing from S3 — mesh override kept until the real asset is uploaded.
    Factory.RegisterMeshOverride(
        TEXT("geo-asset:brisa:e27a9388-84c5-4081-8c85-b7e8abc2b09a"),
        {
            TEXT("/Game/Models/Talude/r59zdz3.r59zdz3"),
            TEXT("/Game/Models/Talude/rs65kp1.rs65kp1"),
            TEXT("/Game/Models/Talude/testtalude.testtalude"),
        }
    );
}
