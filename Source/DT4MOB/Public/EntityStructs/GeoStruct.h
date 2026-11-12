/**
 * @file
 * @brief Shared WGS-84 point struct used by several entity schemas.
 * @ingroup entitystructs
 */
#pragma once

#include "CoreMinimal.h"
#include "GeoStruct.generated.h"

/**
 * @brief A single WGS-84 point, as found under `attributes.location` in several thing types.
 *
 * At present only the deprecated Equivia schemas use it.
 *
 * @ingroup entitystructs
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FGeoLocation
{
    GENERATED_USTRUCT_BODY()

    /** @brief Longitude in decimal degrees (WGS-84). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double longitude = 0.0;

    /** @brief Latitude in decimal degrees (WGS-84). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double latitude = 0.0;
};
