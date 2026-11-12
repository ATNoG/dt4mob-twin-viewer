// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Declares USinalizacaoExtension, the type hooks for "sinalizacao" (road sign) things.
 * @ingroup entitydeps
 */

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "EntityDependencies/Sinalizacao/SinalizacaoModelComponent.h"
#include "SinalizacaoExtension.generated.h"

/**
 * @brief Type hooks for Infraestruturas de Portugal road signs (type key "sinalizacao", struct
 *        FInfPtSinalizacaoData, shown as "IP Sign").
 *
 * Sets the info-tab fields and an amber "SIGN" badge, and attaches
 * USinalizacaoModelComponent to render the real sign face for known sign codes.
 *
 * When tile streaming is off, signs are loaded once with a fetch that ignores geotiles (see
 * RequiresFullFetch() and GetFullFetchFilter()), since there are relatively few of them
 * (roughly 850) and they do not move.
 *
 * @ingroup entitydeps
 */
UCLASS()
class DT4MOB_API USinalizacaoExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    /**
     * @brief Shows the sign code and position.
     * @return Rows for attributes.Code and attributes.location.latitude / longitude.
     */
    virtual TArray<FInfoField> GetDefaultInfoFields() const override;
    /**
     * @brief Amber badge, the same colour as safety barriers.
     * @return Linear colour (0.859, 0.659, 0.337).
     */
    virtual FLinearColor GetBadgeColor() const override;
    /**
     * @brief Fixed badge text.
     * @param TypeKey  Unused.
     * @return         "SIGN".
     */
    virtual FString GetBadgeLabel(const FString& TypeKey) const override;
    /**
     * @brief Opts signs into the one-shot full fetch when tile streaming is disabled.
     * @return Always true.
     */
    virtual bool RequiresFullFetch() const override { return true; }
    /**
     * @brief Builds an RQL filter that matches every thing whose ID contains the type key.
     * @param TypeKey  Registered type key ("sinalizacao").
     * @return         `like(thingId,"*TypeKey*")` with TypeKey substituted.
     */
    virtual FString GetFullFetchFilter(const FString& TypeKey) const override;

    /**
     * @brief Attaches USinalizacaoModelComponent.
     *
     * For sign codes with art in the SignBenchmark atlas, the component draws the real sign
     * face and shape (keyed by attributes.Code) in place of the generic attributes.polygon
     * GLB. Other codes still use the generic GLB load.
     *
     * @return USinalizacaoModelComponent::StaticClass().
     */
    virtual TSubclassOf<UEntityBehaviorComponent> GetBehaviorComponentClass() const override { return USinalizacaoModelComponent::StaticClass(); }
};
