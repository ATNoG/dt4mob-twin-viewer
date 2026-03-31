// Fill out your copyright notice in the Description page of Project Settings.

#include "Services/CoordinatesConversionService.h"
#include "CesiumGeoreference.h"

UCoordinatesConversionService *UCoordinatesConversionService::Instance = nullptr;

UCoordinatesConversionService *UCoordinatesConversionService::Get()
{
    if (!Instance)
    {
        Instance = NewObject<UCoordinatesConversionService>();
        Instance->AddToRoot(); // Prevent garbage collection
        Instance->referenceCoordinates = Instance->ConvertWSG84ToECEF(referenceLatitude, referenceLongitude, referenceAltitude);
    }
    return Instance;
}

FVector3d UCoordinatesConversionService::ConvertWSG84ToECEF(double Latitude, double Longitude, double Altitude)
{
    // Convert degrees to radians
    double latRad = FMath::DegreesToRadians(Latitude);
    double lonRad = FMath::DegreesToRadians(Longitude);

    // WGS84 ellipsoid constants
    const double v = a / FMath::Sqrt(1 - e2 * FMath::Pow(FMath::Sin(latRad), 2)); // Prime vertical radius of curvature

    // Calculate ECEF coordinates
    double X = (v + Altitude) * FMath::Cos(latRad) * FMath::Cos(lonRad);
    double Y = (v + Altitude) * FMath::Cos(latRad) * FMath::Sin(lonRad);
    double Z = ((1 - e2) * v + Altitude) * FMath::Sin(latRad);
    UE_LOG(LogTemp, Warning, TEXT("UCoordinatesConversionService::ConvertWSG84ToECEF: Latitude: %f Longitude: %f Altitude: %f => X: %f Y: %f Z: %f"), Latitude, Longitude, Altitude, X, Y, Z);
    return FVector3d(X, Y, Z);
}

// FVector3d UCoordinatesConversionService::ConvertWSG84ToUELocal(double Latitude, double Longitude, double Altitude)
// {
//     FVector3d ecefCoords = ConvertWSG84ToECEF(Latitude, Longitude, Altitude);
//     // Calculate the difference from the reference point
//     FVector3d delta = ecefCoords - referenceCoordinates;

//     // Convert to UE Local coordinates (assuming ENU: East-North-Up)
//     double latRad = FMath::DegreesToRadians(referenceLatitude);
//     double lonRad = FMath::DegreesToRadians(referenceLongitude);

//     double xLocal = -FMath::Sin(lonRad) * delta.Y + FMath::Cos(lonRad) * delta.X;
//     double yLocal = -FMath::Sin(latRad) * FMath::Cos(lonRad) * delta.X - FMath::Sin(latRad) * FMath::Sin(lonRad) * delta.Y + FMath::Cos(latRad) * delta.Z;
//     double zLocal = FMath::Cos(latRad) * FMath::Cos(lonRad) * delta.X + FMath::Cos(latRad) * FMath::Sin(lonRad) * delta.Y + FMath::Sin(latRad) * delta.Z;

//     UE_LOG(LogTemp, Warning, TEXT("UCoordinatesConversionService::ConvertWSG84ToUELocal: Latitude: %f Longitude: %f Altitude: %f => X_Local: %f Y_Local: %f Z_Local: %f"), Latitude, Longitude, Altitude, xLocal, yLocal, zLocal);
//     // todo multiply by 100 to convert from meters to centimeters used by UE
//     return FVector3d(xLocal, yLocal, zLocal);
// }

// FVector3d UCoordinatesConversionService::ConvertWSG84ToUELocal(double Latitude, double Longitude, double Altitude)
// {
//     FVector3d ecefCoords = ConvertWSG84ToECEF(Latitude, Longitude, Altitude);

//     // Get the CesiumGeoreference from the world
//     ACesiumGeoreference *Georeference = ACesiumGeoreference::GetDefaultGeoreference(GetWorld());
//     if (!Georeference)
//     {
//         UE_LOG(LogTemp, Error, TEXT("CesiumGeoreference not found in world"));
//         return FVector3d::Zero();
//     }

//     // Convert ECEF (in meters) to Cesium local coordinates (in cm)
//     FVector ecefVector = FVector(ecefCoords.X, ecefCoords.Y, ecefCoords.Z);
//     FVector localCoords = Georeference->TransformEarthCenteredEarthFixedPositionToUnreal(ecefVector);

//     FVector3d result(localCoords.X, localCoords.Y, localCoords.Z);
//     UE_LOG(LogTemp, Warning, TEXT("ConvertWSG84ToUELocal: Lat: %f Lon: %f => Local: %s"), Latitude, Longitude, *result.ToString());
//     return result;
// }

FVector UCoordinatesConversionService::ConvertWSG84ToUELocal(double Latitude, double Longitude, double Altitude)
{
    UWorld *World = GWorld;
    if (!World)
    {
        UE_LOG(LogTemp, Error, TEXT("GWorld is null"));
        return FVector::ZeroVector;
    }

    ACesiumGeoreference *Georeference = ACesiumGeoreference::GetDefaultGeoreference(World);
    if (!Georeference)
    {
        UE_LOG(LogTemp, Error, TEXT("CesiumGeoreference not found in world"));
        return FVector::ZeroVector;
    }

    FVector vectorCoords(Longitude, Latitude, Altitude);
    FVector ueVector = Georeference->TransformLongitudeLatitudeHeightPositionToUnreal(vectorCoords);

    UE_LOG(LogTemp, Warning, TEXT("ConvertWSG84ToUELocal: Lat: %f Lon: %f => Local: %s"), Latitude, Longitude, *ueVector.ToString());
    return ueVector;
}
