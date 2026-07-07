#pragma once

#include "CoreMinimal.h"
#include "UI/InfoFieldTypes.h"

class FJsonObject;

class DT4MOB_API FJsonFlattener
{
public:
    /** Flattens a JSON object into leaf-node candidates with dot-paths and display values. */
    static TArray<FInfoFieldCandidate> Flatten(const TSharedPtr<FJsonObject>& Root);

private:
    static void FlattenObject(const TSharedPtr<FJsonObject>& Obj, const FString& Prefix, TArray<FInfoFieldCandidate>& Out);
    static FString PrettifyKey(const FString& Key);
    static FString ScalarToString(const TSharedPtr<FJsonValue>& Val);

    // Top-level keys that carry no useful display value
    static const TSet<FString> SkippedRootKeys;
};
