#include "SignBenchmark/SignSpawnStrategy.h"
#include "SignBenchmark/SignBenchmarkRoot.h"
#include "SignBenchmark/SignAssetService.h"
#include "Services/GlbModelService.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void USignSpawnStrategy::Init(ASignBenchmarkRoot *InRoot, const FString &InBaseUrl, const FSignGridParams &InGrid)
{
    Root = InRoot;
    BaseUrl = InBaseUrl;
    Grid = InGrid;
    Stats = FStats();
    TStart = FPlatformTime::Seconds();
}

void USignSpawnStrategy::Clear()
{
    Root = nullptr;
}

UGlbModelService *USignSpawnStrategy::Glb() const
{
    UWorld *World = Root ? Root->GetWorld() : nullptr;
    UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UGlbModelService>() : nullptr;
}

USignAssetService *USignSpawnStrategy::Assets() const
{
    UWorld *World = Root ? Root->GetWorld() : nullptr;
    UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<USignAssetService>() : nullptr;
}

void USignSpawnStrategy::Arm(int32 Count, TFunction<void()> OnAllReady)
{
    PendingCount = Count;
    bLatchFired = false;
    LatchCallback = MoveTemp(OnAllReady);

    if (PendingCount <= 0)
    {
        bLatchFired = true;
        if (LatchCallback)
            LatchCallback();
    }
}

void USignSpawnStrategy::Signal()
{
    if (bLatchFired)
        return;

    if (--PendingCount <= 0)
    {
        bLatchFired = true;
        if (LatchCallback)
            LatchCallback();
    }
}
