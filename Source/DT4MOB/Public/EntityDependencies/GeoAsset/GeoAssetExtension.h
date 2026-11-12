// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UGeoAssetExtension, the type hooks for "geo-asset" (geotechnical asset) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "GeoAssetExtension.generated.h"

/**
 * @brief Type hooks for geotechnical assets such as retaining walls and slopes (type key
 *        "geo-asset", struct FGeoAssetData, shown as "Geo Asset").
 *
 * Only sets the info-tab fields and a green "GEO" badge. Thing IDs that also contain
 * ".instrument." resolve to UGeoInstrumentExtension instead, because the factory prefers the
 * longest matching type key.
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UGeoAssetExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the asset registration, location, position, condition index and size.
     * @return Rows for attributes.matricula, localizacao, latitude, longitude,
     *         indiceCondicaoAtual, altura and extensao.
     */
    virtual TArray<FInfoField> GetDefaultInfoFields() const override;
    /**
     * @brief Green badge, the colour shared by point-like infrastructure types.
     * @return Linear colour (0.557, 0.882, 0.533).
     */
    virtual FLinearColor GetBadgeColor() const override;
    /**
     * @brief Fixed badge text.
     * @param TypeKey  Unused.
     * @return         "GEO".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override;
};
