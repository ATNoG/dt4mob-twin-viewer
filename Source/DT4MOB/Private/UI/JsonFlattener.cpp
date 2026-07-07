#include "UI/JsonFlattener.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

const TSet<FString> FJsonFlattener::SkippedRootKeys = { TEXT("thingId"), TEXT("policyId") };

TArray<FInfoFieldCandidate> FJsonFlattener::Flatten(const TSharedPtr<FJsonObject>& Root)
{
    TArray<FInfoFieldCandidate> Out;
    if (Root.IsValid())
        FlattenObject(Root, FString(), Out);
    return Out;
}

void FJsonFlattener::FlattenObject(const TSharedPtr<FJsonObject>& Obj, const FString& Prefix, TArray<FInfoFieldCandidate>& Out)
{
    for (const auto& Pair : Obj->Values)
    {
        if (Prefix.IsEmpty() && SkippedRootKeys.Contains(Pair.Key))
            continue;

        const FString Path = Prefix.IsEmpty() ? Pair.Key : Prefix + TEXT(".") + Pair.Key;

        switch (Pair.Value->Type)
        {
        case EJson::Object:
            FlattenObject(Pair.Value->AsObject(), Path, Out);
            break;

        case EJson::Array:
        {
            const TArray<TSharedPtr<FJsonValue>>& Arr = Pair.Value->AsArray();
            bool bHasObjects = false;
            for (const auto& Item : Arr)
                if (Item->Type == EJson::Object || Item->Type == EJson::Array)
                    { bHasObjects = true; break; }

            FString Display;
            if (bHasObjects || Arr.Num() > 5)
            {
                Display = FString::Printf(TEXT("[%d items]"), Arr.Num());
            }
            else if (Arr.Num() == 0)
            {
                Display = TEXT("[]");
            }
            else
            {
                TArray<FString> Parts;
                for (const auto& Item : Arr)
                    Parts.Add(ScalarToString(Item));
                Display = FString::Join(Parts, TEXT(", "));
            }

            Out.Add({ Path, PrettifyKey(Pair.Key), Display });
            break;
        }

        case EJson::Null:
            Out.Add({ Path, PrettifyKey(Pair.Key), TEXT("null") });
            break;

        default:
            Out.Add({ Path, PrettifyKey(Pair.Key), ScalarToString(Pair.Value) });
            break;
        }
    }
}

FString FJsonFlattener::ScalarToString(const TSharedPtr<FJsonValue>& Val)
{
    switch (Val->Type)
    {
    case EJson::String:  return Val->AsString();
    case EJson::Boolean: return Val->AsBool() ? TEXT("true") : TEXT("false");
    case EJson::Number:
    {
        const double N = Val->AsNumber();
        return (FMath::Frac(N) == 0.0 && FMath::Abs(N) < 1e9)
            ? FString::FromInt((int32)N)
            : FString::SanitizeFloat(N, 4);
    }
    default: return FString();
    }
}

FString FJsonFlattener::PrettifyKey(const FString& Key)
{
    // Split camelCase and capitalise first letter
    FString Result;
    for (int32 i = 0; i < Key.Len(); i++)
    {
        const TCHAR C = Key[i];
        if (i == 0)
            Result += FChar::ToUpper(C);
        else if (FChar::IsUpper(C) && !FChar::IsUpper(Key[i - 1]))
        {
            Result += TEXT(' ');
            Result += C;
        }
        else if (C == '_' || C == '-')
            Result += TEXT(' ');
        else
            Result += C;
    }
    return Result;
}
