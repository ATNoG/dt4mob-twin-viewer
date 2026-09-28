#include "SignBenchmark/SignBenchmarkGameMode.h"
#include "SignBenchmark/SignBenchmarkRoot.h"
#include "SignBenchmark/SignSpawnStrategy.h"
#include "SignBenchmark/SignSpawnStrategy_PerSign.h"
#include "SignBenchmark/SignSpawnStrategy_Atlas.h"
#include "SignBenchmark/SignSpawnStrategy_PerTexture.h"
#include "SignBenchmark/SignAssetService.h"
#include "Services/GlbModelService.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "HAL/FileManager.h"

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

    LastStrategyName = StrategyName;
    LastRequestedCount = Count;
    LastCacheState = bNextRunIsCold ? TEXT("Cold") : TEXT("Warm");
    bNextRunIsCold = false;

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
    // Cancel any pending settle/capture timers from a run that hasn't finished measuring
    // yet, so they don't fire later against a strategy this call is about to null out.
    if (UWorld *World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SettleTimer);
        World->GetTimerManager().ClearTimer(CaptureTimer);
    }

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

void ASignBenchmarkGameMode::SignBenchSetCold()
{
    // Two layers to clear, not one. The on-disk directories are the *second* line of
    // defense; RequestMesh/RequestTexture/RequestAtlas all check an in-memory,
    // session-only TMap FIRST and return instantly with zero I/O on a hit (see
    // UGlbModelService::RequestMesh / USignAssetService::RequestTexture/RequestAtlas).
    // Deleting files on disk does not touch those in-memory maps at all, so without
    // ClearSessionCache() a second cold run in the same session would silently hit the
    // first run's already-loaded objects and report a fast "Cold" result that wasn't
    // actually cold — this was caught by testing, not anticipated up front.
    UGameInstance *GI = GetGameInstance();
    if (UGlbModelService *Glb = GI ? GI->GetSubsystem<UGlbModelService>() : nullptr)
        Glb->ClearSessionCache();
    if (USignAssetService *Assets = GI ? GI->GetSubsystem<USignAssetService>() : nullptr)
        Assets->ClearSessionCache();

    // Saved/GlbCache is the production UGlbModelService cache, shared with the live
    // viewer — deleting it is harmless (it just redownloads meshes next normal launch)
    // but is not benchmark-exclusive, worth knowing.
    const FString SignCacheDir = FPaths::ProjectSavedDir() / TEXT("SignBenchCache");
    const FString GlbCacheDir = FPaths::ProjectSavedDir() / TEXT("GlbCache");

    IFileManager &FM = IFileManager::Get();
    const bool bSignDeleted = !FM.DirectoryExists(*SignCacheDir) || FM.DeleteDirectory(*SignCacheDir, false, true);
    const bool bGlbDeleted = !FM.DirectoryExists(*GlbCacheDir) || FM.DeleteDirectory(*GlbCacheDir, false, true);

    if (!bSignDeleted || !bGlbDeleted)
    {
        UE_LOG(LogTemp, Error, TEXT("SignBench: failed to clear cache dir(s) (SignBenchCache: %s, GlbCache: %s) — next run will NOT be genuinely cold"),
               bSignDeleted ? TEXT("ok") : TEXT("FAILED"), bGlbDeleted ? TEXT("ok") : TEXT("FAILED"));
        if (GEngine)
            GEngine->AddOnScreenDebugMessage(0, 8.0f, FColor::Red, TEXT("SignBench: cache clear FAILED — see log"));
        return;
    }

    bNextRunIsCold = true;
    UE_LOG(LogTemp, Display, TEXT("SignBench: session + disk caches cleared, next SignBenchSpawn will be recorded as Cold"));
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(0, 5.0f, FColor::Cyan, TEXT("SignBench: caches cleared, next run = Cold"));
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
    LastResultHeader = Header;

    if (GEngine)
    {
        for (int32 i = 0; i < Lines.Num(); ++i)
            GEngine->AddOnScreenDebugMessage(100 + i, 30.0f, FColor::Green, Lines[i]);
    }
    UE_LOG(LogTemp, Display, TEXT("%s"), *LastReport);

    // Structural stats are ready now, but frame/draw/GPU stats need the scene to reach
    // steady state first — the CSV row is written once that capture window closes
    // (EndEngineProfilingCapture), not here, so both halves land in one consistent row.
    GetWorld()->GetTimerManager().SetTimer(
        SettleTimer, this, &ASignBenchmarkGameMode::BeginEngineProfilingCapture, SettleDelaySeconds, false);
}

void ASignBenchmarkGameMode::BeginEngineProfilingCapture()
{
    if (!GEngine)
        return;

    // CSV Profiler, not `stat startfile` (that writes a binary .uestats file readable
    // only via Session Frontend, not a CSV — verified against Epic's docs after an
    // earlier wrong assumption here). `csvprofile start/stop` writes an actual CSV to
    // Saved/Profiling/CSV/, with unit stats (frame/game/render/GPU time) and free
    // memory captured by default. Same command you'd type by hand at the console —
    // this just times the window around steady state instead of reacting to it manually.
    GEngine->Exec(GetWorld(), TEXT("csvprofile start"));
    ProfilingWindowStart = FDateTime::Now();

    UE_LOG(LogTemp, Display, TEXT("SignBench: engine-stats capture window opened (%s)"), *ProfilingWindowStart.ToIso8601());

    GetWorld()->GetTimerManager().SetTimer(
        CaptureTimer, this, &ASignBenchmarkGameMode::EndEngineProfilingCapture, CaptureWindowSeconds, false);
}

void ASignBenchmarkGameMode::EndEngineProfilingCapture()
{
    if (GEngine)
        GEngine->Exec(GetWorld(), TEXT("csvprofile stop"));

    ProfilingWindowEnd = FDateTime::Now();
    UE_LOG(LogTemp, Display, TEXT("SignBench: engine-stats capture window closed (%s) — see newest file under Saved/Profiling/CSV/"), *ProfilingWindowEnd.ToIso8601());

    AppendResultsCsvRow();
}

void ASignBenchmarkGameMode::AppendResultsCsvRow()
{
    if (!Active)
        return;

    const USignSpawnStrategy::FStats &S = Active->GetStats();

    int64 StaticMeshBytes = 0;
    int64 Texture2DBytes = 0;
    Active->GetAssetMemoryBytes(StaticMeshBytes, Texture2DBytes);
    const double StaticMeshMB = StaticMeshBytes / (1024.0 * 1024.0);
    const double Texture2DMB = Texture2DBytes / (1024.0 * 1024.0);

    const FString CsvPath = FPaths::ProjectSavedDir() / TEXT("SignBenchmark/benchmark_results.csv");

    if (!FPaths::FileExists(CsvPath))
    {
        const FString HeaderRow = TEXT("Timestamp,Strategy,RequestedCount,CacheState,Result,InstanceCount,ComponentCount,UniqueMeshCount,UniqueMaterialCount,UniqueTextureCount,PrepareMs,SpawnMs,StaticMesh_MB,Texture2D_MB,ProfilingWindowStart,ProfilingWindowEnd\n");
        FFileHelper::SaveStringToFile(HeaderRow, *CsvPath);
    }

    const FString Row = FString::Printf(
        TEXT("%s,%s,%d,%s,%s,%d,%d,%d,%d,%d,%.3f,%.3f,%.3f,%.3f,%s,%s\n"),
        *FDateTime::Now().ToIso8601(),
        *LastStrategyName,
        LastRequestedCount,
        *LastCacheState,
        *LastResultHeader,
        S.InstanceCount, S.ComponentCount, S.UniqueMeshCount, S.UniqueMaterialCount, S.UniqueTextureCount,
        S.PrepareSeconds * 1000.0, S.SpawnSeconds * 1000.0,
        StaticMeshMB, Texture2DMB,
        *ProfilingWindowStart.ToIso8601(), *ProfilingWindowEnd.ToIso8601());

    FFileHelper::SaveStringToFile(Row, *CsvPath, FFileHelper::EEncodingOptions::AutoDetect,
        &IFileManager::Get(), FILEWRITE_Append);

    UE_LOG(LogTemp, Display, TEXT("SignBench: appended result row to %s"), *CsvPath);
}
