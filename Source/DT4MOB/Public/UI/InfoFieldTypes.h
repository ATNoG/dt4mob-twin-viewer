#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "InfoFieldTypes.generated.h"

/** One row in the Info tab: a display label, a dot-path into RawJson, and an optional unit suffix. */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfoField
{
    GENERATED_BODY()

    /** Label shown on the left side of the row, e.g. "Speed". */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString DisplayName;

    /** Dot-separated path into the entity's RawJson, e.g. "features.state.properties.speed". */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString JsonDotPath;

    /** Optional suffix appended after the value, e.g. "m/s", "°", "%". */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString Unit;

    FInfoField() {}
    FInfoField(const FString& InName, const FString& InPath, const FString& InUnit = FString())
        : DisplayName(InName), JsonDotPath(InPath), Unit(InUnit) {}
};

/** Wrapper so TMap<FString, TArray<FInfoField>> serialises correctly via SaveGame. */
USTRUCT()
struct FInfoFieldList
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FInfoField> Fields;
};

UCLASS()
class DT4MOB_API UEntityInfoSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    /** Maps TypeKey (e.g. "traci", "fire:") to the user's chosen field list. */
    UPROPERTY()
    TMap<FString, FInfoFieldList> FieldsByType;
};
