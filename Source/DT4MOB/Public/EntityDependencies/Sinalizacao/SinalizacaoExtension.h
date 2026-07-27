// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "SinalizacaoExtension.generated.h"

/**
 * @brief Type-specific behavior for "sinalizacao" (road sign) entities.
 *
 * Signs are currently loaded via a one-shot unfiltered-by-geotile fetch (see
 * RequiresFullFetch/GetFullFetchFilter) rather than tile streaming, since there
 * are relatively few of them (~850) and they don't move.
 */
UCLASS()
class DT4MOB_API USinalizacaoExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual TArray<FInfoField> GetDefaultInfoFields() const override;
    virtual FLinearColor GetBadgeColor() const override;
    virtual FString GetBadgeLabel(const FString& TypeKey) const override;
    virtual bool RequiresFullFetch() const override { return true; }
    virtual FString GetFullFetchFilter(const FString& TypeKey) const override;
};
