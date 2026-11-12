/**
 * @file
 * @brief Declares FInfoField, FInfoFieldList, FInfoFieldCandidate and UEntityInfoSaveGame.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "InfoFieldTypes.generated.h"

/** @brief One row in the Info tab: a display label, a dot-path into RawJson, and an optional unit suffix. @ingroup ui */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfoField
{
    GENERATED_BODY()

    /** @brief Label shown on the left side of the row, e.g. "Speed". */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString DisplayName;

    /** @brief Dot-separated path into the entity's RawJson, e.g. "features.state.properties.speed". */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString JsonDotPath;

    /** @brief Optional suffix appended after the value, e.g. "m/s" or "%". */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FString Unit;

    /** @brief Creates an empty field. */
    FInfoField() {}

    /**
     * @brief Creates a field.
     * @param InName Label.
     * @param InPath Dot-path into RawJson.
     * @param InUnit Optional unit suffix.
     */
    FInfoField(const FString& InName, const FString& InPath, const FString& InUnit = FString())
        : DisplayName(InName), JsonDotPath(InPath), Unit(InUnit) {}
};

/** @brief Wrapper so TMap<FString, TArray<FInfoField>> serialises correctly via SaveGame. @ingroup ui */
USTRUCT()
struct FInfoFieldList
{
    GENERATED_BODY()

    /** @brief The fields, in display order. */
    UPROPERTY()
    TArray<FInfoField> Fields;
};

/** @brief A candidate field produced by flattening an entity's JSON: used by the config panel. @ingroup ui */
USTRUCT(BlueprintType)
struct DT4MOB_API FInfoFieldCandidate
{
    GENERATED_BODY()

    FString DotPath;        ///< Path into the JSON, e.g. "attributes.matricula".
    FString SuggestedLabel; ///< Prettified last key segment, e.g. "Matricula".
    FString CurrentValue;   ///< Value from the JSON for context, e.g. "I3 MEEC" or "[9 items]".
    bool bMissing = false;  ///< True when the field comes from the type's defaults but not from this entity's JSON.
};

/**
 * @brief Save game that stores the user's Info tab field selection per entity type
 *        (slot "EntityInfoFields", written by UInfoFieldRegistry).
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UEntityInfoSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    /** @brief Maps TypeKey (e.g. "traci", "fire:") to the user's chosen field list. */
    UPROPERTY()
    TMap<FString, FInfoFieldList> FieldsByType;
};
