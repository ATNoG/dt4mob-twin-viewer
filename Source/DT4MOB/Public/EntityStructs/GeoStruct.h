#pragma once

#include "CoreMinimal.h"
#include "GeoStruct.generated.h"

/**
 * @brief Shared geographic location struct used across multiple entity types.
 *
 * Stores a WGS-84 coordinate pair. Used as a common sub-struct wherever entities
 * expose a single point geographic location.
 */
USTRUCT(BlueprintType)
struct DT4MOB_API FGeoLocation
{
    GENERATED_USTRUCT_BODY()

    /** @brief Geographic longitude in decimal degrees (WGS-84). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double longitude = 0.0;

    /** @brief Geographic latitude in decimal degrees (WGS-84). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    double latitude = 0.0;
};
