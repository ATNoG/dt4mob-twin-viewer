// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares UGeoInstrumentExtension, the type hooks for ".instrument." (geotechnical
 *        monitoring instrument) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "GeoInstrumentExtension.generated.h"

/**
 * @brief Type hooks for geotechnical monitoring instruments attached to geo assets (type key
 *        ".instrument.", struct FGeoInstrumentData, shown as "Geo Instrument").
 *
 * Only sets the info-tab fields and a green "INSTR" badge. The key is longer than
 * "geo-asset", so instrument thing IDs that contain both resolve here.
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API UGeoInstrumentExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the instrument registration, instrument type and position.
     * @return Rows for attributes.matricula, tipoInstrumento.name and
     *         coordinates.latitude / coordinates.longitude.
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
     * @return         "INSTR".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override;
};
