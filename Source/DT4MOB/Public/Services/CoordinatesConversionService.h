// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "CoordinatesConversionService.generated.h"

/**
 * @brief Singleton service for converting WGS-84 geographic coordinates to Unreal Engine world space.
 *
 * Uses a CesiumGeoreference actor present in the current world to perform the final
 * conversion, so a valid ACesiumGeoreference must exist at runtime.
 *
 * Access the singleton via UCoordinatesConversionService::Get(). The instance is added
 * to the GC root to prevent it from being collected.
 *
 * @note The reference point constants (referenceLatitude, referenceLongitude) are used only
 *       by the commented-out ENU implementation and are retained for future use.
 */
UCLASS()
class DT4MOB_API UCoordinatesConversionService : public UObject
{
	GENERATED_BODY()

private:
	/** @brief Lazily-created singleton instance. Added to the GC root on first access. */
	static UCoordinatesConversionService *Instance;

	/** @brief Reference latitude used for local ENU coordinate calculations (degrees). */
	static constexpr double referenceLatitude = 39.885078;

	/** @brief Reference longitude used for local ENU coordinate calculations (degrees). */
	static constexpr double referenceLongitude = -8.212414;

	/** @brief Reference altitude used for local ENU coordinate calculations (metres). */
	static constexpr double referenceAltitude = 0.0;

	// WGS-84 ellipsoid constants

	/** @brief WGS-84 semi-major axis in metres. */
	static constexpr double a = 6378137.0;

	/** @brief Square of the WGS-84 first eccentricity. */
	static constexpr double e2 = 1.0 - (1.0 - 1.0 / 298.257223563) * (1.0 - 1.0 / 298.257223563);

	/** @brief Pre-computed ECEF position of the reference point; cached on first Get() call. */
	FVector3d referenceCoordinates = FVector3d::Zero();

public:
	/**
	 * @brief Returns the singleton instance, creating it on first call.
	 *
	 * The instance is rooted against garbage collection.
	 *
	 * @return Non-null pointer to the shared UCoordinatesConversionService instance.
	 */
	static UCoordinatesConversionService *Get();

	/**
	 * @brief Converts a WGS-84 geodetic position to Earth-Centred Earth-Fixed (ECEF) Cartesian coordinates.
	 *
	 * @param Latitude   Geodetic latitude in decimal degrees.
	 * @param Longitude  Geodetic longitude in decimal degrees.
	 * @param Altitude   Height above the WGS-84 ellipsoid in metres.
	 * @return ECEF position vector in metres.
	 */
	FVector3d ConvertWSG84ToECEF(double Latitude, double Longitude, double Altitude);

	/**
	 * @brief Converts a WGS-84 geodetic position to Unreal Engine world-space coordinates.
	 *
	 * Delegates to the ACesiumGeoreference present in GWorld via
	 * TransformLongitudeLatitudeHeightPositionToUnreal().
	 *
	 * @param Latitude   Geodetic latitude in decimal degrees.
	 * @param Longitude  Geodetic longitude in decimal degrees.
	 * @param Altitude   Height above the WGS-84 ellipsoid in metres.
	 * @return Unreal Engine world-space position (in centimetres).
	 */
	FVector ConvertWSG84ToUELocal(double Latitude, double Longitude, double Altitude);
};
