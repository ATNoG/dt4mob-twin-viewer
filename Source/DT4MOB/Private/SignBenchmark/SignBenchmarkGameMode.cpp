#include "SignBenchmark/SignBenchmarkGameMode.h"
#include "SignBenchmark/SignBenchmarkRoot.h"
#include "SignBenchmark/SignSpawnStrategy.h"
#include "SignBenchmark/SignSpawnStrategy_PerSign.h"
#include "SignBenchmark/SignSpawnStrategy_Atlas.h"
#include "SignBenchmark/SignSpawnStrategy_PerTexture.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

ASignBenchmarkGameMode::ASignBenchmarkGameMode()
{
    // DefaultPawnClass/PlayerControllerClass left as engine defaults (free-fly
    // DefaultPawn) — no project pawn dependency on this standalone test level.
}

void ASignBenchmarkGameMode::SignBenchSpawn(const FString &StrategyName, int32 Count)
{
    ESignStrategy Strategy;
    if (!SignBenchmark::ParseStrategy(StrategyName, Strategy))
    {
        const FString Msg = FString::Printf(TEXT("SignBench: unknown strategy '%s' (use PerSign / AtlasMaterial / PerTexture)"), *StrategyName);
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(0, 8.0f, FColor::Red, Msg);
        UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
        return;
    }

    SignBenchClear();

    Root = GetWorld()->SpawnActor<ASignBenchmarkRoot>();

    switch (Strategy)
    {
        case ESignStrategy::PerSign:
        {
            Active = NewObject<USignSpawnStrategy_PerSign>(this);
            break;
        }
        case ESignStrategy::AtlasMaterial:
        {
            USignSpawnStrategy_Atlas *AtlasStrategy = NewObject<USignSpawnStrategy_Atlas>(this);
            AtlasStrategy->AtlasBaseMaterial = AtlasBaseMaterial.LoadSynchronous();
            Active = AtlasStrategy;
            break;
        }
        case ESignStrategy::PerTexture:
        {
            USignSpawnStrategy_PerTexture *TexStrategy = NewObject<USignSpawnStrategy_PerTexture>(this);
            TexStrategy->TexturedBaseMaterial = TexturedBaseMaterial.LoadSynchronous();
            Active = TexStrategy;
            break;
        }
    }

    if (!Active)
    {
        UE_LOG(LogTemp, Error, TEXT("SignBench: failed to create strategy for '%s'"), *StrategyName);
        return;
    }

    Active->Init(Root, BaseUrl, Grid);

    const TArray<FString> CodeSequence = SignBenchmark::BuildCodeSequence(Count);
    Active->Execute(CodeSequence, [this](bool bSuccess)
    {
        ReportStats(bSuccess ? TEXT("OK") : TEXT("FAILED"));
    });
}

void ASignBenchmarkGameMode::SignBenchClear()
{
    if (Active)
    {
        Active->Clear();
        Active = nullptr;
    }
    if (Root)
    {
        Root->Destroy();
        Root = nullptr;
    }
}

void ASignBenchmarkGameMode::SignBenchInfo()
{
    if (LastReport.IsEmpty())
    {
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Yellow, TEXT("SignBench: no run yet"));
        return;
    }

    TArray<FString> Lines;
    LastReport.ParseIntoArrayLines(Lines);
    if (GEngine)
    {
        for (int32 i = 0; i < Lines.Num(); ++i)
            GEngine->AddOnScreenDebugMessage(100 + i, 30.0f, FColor::Green, Lines[i]);
    }
    UE_LOG(LogTemp, Display, TEXT("%s"), *LastReport);
}

void ASignBenchmarkGameMode::SignBenchGrid(int32 Columns, float SpacingX, float SpacingY)
{
    Grid.Columns = FMath::Max(1, Columns);
    Grid.SpacingX = SpacingX;
    Grid.SpacingY = SpacingY;
    UE_LOG(LogTemp, Display, TEXT("SignBench: grid set to %d columns, spacing (%.0f, %.0f)"), Grid.Columns, Grid.SpacingX, Grid.SpacingY);
}

void ASignBenchmarkGameMode::SignBenchUrl(const FString &NewBaseUrl)
{
    BaseUrl = NewBaseUrl;
    UE_LOG(LogTemp, Display, TEXT("SignBench: base URL set to %s"), *BaseUrl);
}

void ASignBenchmarkGameMode::ReportStats(const FString &Header)
{
    if (!Active)
        return;

    const USignSpawnStrategy::FStats &S = Active->GetStats();

    TArray<FString> Lines;
    Lines.Add(FString::Printf(TEXT("SignBench [%s]"), *Header));
    Lines.Add(FString::Printf(TEXT("Instances: %d  Components: %d"), S.InstanceCount, S.ComponentCount));
    Lines.Add(FString::Printf(TEXT("Unique meshes: %d  materials: %d  textures: %d"),
                               S.UniqueMeshCount, S.UniqueMaterialCount, S.UniqueTextureCount));
    Lines.Add(FString::Printf(TEXT("Prepare: %.1f ms  Spawn: %.1f ms"), S.PrepareSeconds * 1000.0, S.SpawnSeconds * 1000.0));

    LastReport = FString::Join(Lines, TEXT("\n"));

    if (GEngine)
    {
        for (int32 i = 0; i < Lines.Num(); ++i)
            GEngine->AddOnScreenDebugMessage(100 + i, 30.0f, FColor::Green, Lines[i]);
    }
    UE_LOG(LogTemp, Display, TEXT("%s"), *LastReport);
}
