/**
 * @file
 * @brief Declares FJsonFlattener, which turns entity JSON into a flat list of fields.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "UI/InfoFieldTypes.h"

class FJsonObject;

/**
 * @brief Turns an entity's JSON into a flat list of fields for the Info tab configuration panel.
 * @ingroup ui
 */
class DT4MOB_API FJsonFlattener
{
public:
    /**
     * @brief Flattens a JSON object into leaf-node candidates with dot-paths and display values.
     *
     * Objects are walked recursively. An array becomes one candidate: its values joined with
     * commas if it holds up to 5 scalars, otherwise "[N items]". The root keys thingId and
     * policyId are skipped.
     *
     * @param Root Entity JSON.
     * @return One candidate per leaf, in JSON order.
     */
    static TArray<FInfoFieldCandidate> Flatten(const TSharedPtr<FJsonObject>& Root);

private:
    /**
     * @brief Adds the leaves of one object to Out.
     * @param Obj    Object to walk.
     * @param Prefix Dot-path of Obj, empty for the root.
     * @param Out    Receives the candidates.
     */
    static void FlattenObject(const TSharedPtr<FJsonObject>& Obj, const FString& Prefix, TArray<FInfoFieldCandidate>& Out);

    /**
     * @brief Turns a JSON key into a readable label.
     * @param Key JSON key.
     * @return The label.
     */
    static FString PrettifyKey(const FString& Key);

    /**
     * @brief Formats a scalar JSON value as text.
     * @param Val Value to format.
     * @return The text.
     */
    static FString ScalarToString(const TSharedPtr<FJsonValue>& Val);

    /** @brief Top-level keys that carry no useful display value (thingId, policyId). */
    static const TSet<FString> SkippedRootKeys;
};
