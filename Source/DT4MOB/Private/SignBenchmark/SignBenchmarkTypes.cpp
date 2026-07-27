#include "SignBenchmark/SignBenchmarkTypes.h"
#include "Engine/StaticMesh.h"

namespace SignBenchmark
{
    FTransform ComputeGridTransform(int32 Index, const FSignGridParams &Params)
    {
        const int32 Columns = FMath::Max(1, Params.Columns);
        const int32 Row = Index / Columns;
        const int32 Col = Index % Columns;

        const FVector Location = Params.Origin + FVector(Col * Params.SpacingX, Row * Params.SpacingY, 0.0f);
        return FTransform(FRotator(0.0f, Params.Yaw, 0.0f), Location, FVector::OneVector);
    }

    const TArray<FString> &CircleCodes()
    {
        static const TArray<FString> Codes = {
            TEXT("C1"), TEXT("C11B"), TEXT("C13"), TEXT("C14A"), TEXT("C15"), TEXT("C20A"), TEXT("C4E"),
            TEXT("D1A"), TEXT("D1C"), TEXT("D8")
        };
        return Codes;
    }

    const TArray<FString> &TriangleCodes()
    {
        static const TArray<FString> Codes = {
            TEXT("A12"), TEXT("A16A"), TEXT("A1A"), TEXT("A1B"), TEXT("A22"), TEXT("A30"), TEXT("A4B")
        };
        return Codes;
    }

    ESignShape ShapeForCode(const FString &Code)
    {
        return CircleCodes().Contains(Code) ? ESignShape::Circle : ESignShape::Triangle;
    }

    FString ShapeName(ESignShape Shape)
    {
        return Shape == ESignShape::Circle ? TEXT("circle") : TEXT("triangle");
    }

    TArray<FString> BuildCodeSequence(int32 TargetCount)
    {
        TArray<FString> AllCodes;
        AllCodes.Append(CircleCodes());
        AllCodes.Append(TriangleCodes());

        TArray<FString> Sequence;
        Sequence.Reserve(FMath::Max(0, TargetCount));
        for (int32 i = 0; i < TargetCount; ++i)
            Sequence.Add(AllCodes[i % AllCodes.Num()]);

        return Sequence;
    }

    int32 FindSignFaceSlot(const UStaticMesh *Mesh)
    {
        if (!Mesh)
            return 1;

        const TArray<FStaticMaterial> &Materials = Mesh->GetStaticMaterials();
        for (int32 i = 0; i < Materials.Num(); ++i)
        {
            const FString SlotName = Materials[i].MaterialSlotName.ToString();
            if (SlotName.Contains(TEXT("SignFace")))
                return i;
        }

        UE_LOG(LogTemp, Warning, TEXT("SignBenchmark: no material slot containing 'SignFace' found on %s, falling back to slot 1"),
               *Mesh->GetName());
        return 1;
    }

    FString StrategyToString(ESignStrategy Strategy)
    {
        switch (Strategy)
        {
            case ESignStrategy::PerSign: return TEXT("PerSign");
            case ESignStrategy::AtlasMaterial: return TEXT("AtlasMaterial");
            case ESignStrategy::PerTexture: return TEXT("PerTexture");
            default: return TEXT("Unknown");
        }
    }

    bool ParseStrategy(const FString &In, ESignStrategy &OutStrategy)
    {
        if (In.Equals(TEXT("PerSign"), ESearchCase::IgnoreCase))
        {
            OutStrategy = ESignStrategy::PerSign;
            return true;
        }
        if (In.Equals(TEXT("AtlasMaterial"), ESearchCase::IgnoreCase) || In.Equals(TEXT("Atlas"), ESearchCase::IgnoreCase))
        {
            OutStrategy = ESignStrategy::AtlasMaterial;
            return true;
        }
        if (In.Equals(TEXT("PerTexture"), ESearchCase::IgnoreCase))
        {
            OutStrategy = ESignStrategy::PerTexture;
            return true;
        }
        return false;
    }
}
