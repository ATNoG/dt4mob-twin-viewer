// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityTypeExtension.h"
#include "InfoRouteLnExtension.generated.h"

/** @brief Type-specific behavior for "info-route-ln" (route info line) entities. */
UCLASS()
class DT4MOB_API UInfoRouteLnExtension : public UEntityTypeExtension
{
    GENERATED_BODY()

public:
    virtual TArray<FInfoField> GetDefaultInfoFields() const override
    {
        return {
            { "Route",      "attributes.route_name" },
            { "Road No.",   "attributes.roadnumber" },
            { "Km Start",   "attributes.kmi"        },
            { "Km End",     "attributes.kmf"        },
            { "Length",     "attributes.Shape_Length" },
        };
    }
    virtual FLinearColor GetBadgeColor() const override { return FLinearColor(0.400f, 0.800f, 0.850f); } // cyan
    virtual FString GetBadgeLabel(const FString& TypeKey) const override { return TEXT("ROUTE"); }
};
