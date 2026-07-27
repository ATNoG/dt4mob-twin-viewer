#include "SignBenchmark/SignSpawnStrategy_PerSign.h"
#include "SignBenchmark/SignBenchmarkRoot.h"
#include "SignBenchmark/SignGlbRequestProxy.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

void USignSpawnStrategy_PerSign::Execute(const TArray<FString> &CodeSequence, TFunction<void(bool)> OnComplete)
{
    MeshByCode.Reset();

    TArray<FString> UniqueCodes;
    for (const FString &Code : CodeSequence)
        UniqueCodes.AddUnique(Code);

    if (UniqueCodes.IsEmpty())
    {
        OnComplete(false);
        return;
    }

    Arm(UniqueCodes.Num(), [this, CodeSequence, OnComplete]()
    {
        TAssetsReady = FPlatformTime::Seconds();
        Stats.PrepareSeconds = TAssetsReady - TStart;
        const double SpawnStart = FPlatformTime::Seconds();

        for (int32 i = 0; i < CodeSequence.Num(); ++i)
        {
            TObjectPtr<UStaticMesh> *MeshPtr = MeshByCode.Find(CodeSequence[i]);
            if (!MeshPtr || !*MeshPtr)
                continue;

            UStaticMeshComponent *Comp = NewObject<UStaticMeshComponent>(Root);
            Comp->SetStaticMesh(*MeshPtr);
            Comp->SetMobility(EComponentMobility::Static);
            Comp->SetupAttachment(Root->GetRootComponent());
            // Set the transform BEFORE registering — moving an already-registered
            // Static-mobility component logs a "has to be 'Movable'" warning.
            Comp->SetRelativeTransform(SignBenchmark::ComputeGridTransform(i, Grid));
            Comp->RegisterComponent();

            ++Stats.ComponentCount;
            ++Stats.InstanceCount;
        }

        Stats.UniqueMeshCount = MeshByCode.Num();
        Stats.UniqueMaterialCount = 0; // baked into each glb, no runtime materials created
        Stats.UniqueTextureCount = 0;  // baked into each glb, no runtime textures created
        Stats.SpawnSeconds = FPlatformTime::Seconds() - SpawnStart;

        OnComplete(true);
    });

    for (const FString &Code : UniqueCodes)
    {
        const FString Url = BaseUrl + TEXT("PerSign/") + Code + TEXT(".glb");
        USignGlbRequestProxy::RequestMesh(this, Url, [this, Code](const FString &, UStaticMesh *Mesh)
        {
            if (Mesh)
                MeshByCode.Add(Code, Mesh);
            else
                UE_LOG(LogTemp, Warning, TEXT("SignBench PerSign: failed to load mesh for code %s"), *Code);
            Signal();
        });
    }
}

void USignSpawnStrategy_PerSign::Clear()
{
    MeshByCode.Reset();
    Super::Clear();
}
