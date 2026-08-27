#include "SignBenchmark/SignSpawnStrategy_PerTexture.h"
#include "SignBenchmark/SignBenchmarkRoot.h"
#include "SignBenchmark/SignGlbRequestProxy.h"
#include "SignBenchmark/SignAssetService.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"

void USignSpawnStrategy_PerTexture::Execute(const TArray<FString> &CodeSequence, TFunction<void(bool)> OnComplete)
{
    MeshByShape.Reset();
    TexByCode.Reset();
    ISMByCode.Reset();

    if (!TexturedBaseMaterial)
    {
        UE_LOG(LogTemp, Error, TEXT("SignBench PerTexture: TexturedBaseMaterial not set"));
        OnComplete(false);
        return;
    }

    TArray<FString> UniqueCodes;
    for (const FString &Code : CodeSequence)
        UniqueCodes.AddUnique(Code);

    TArray<ESignShape> ShapesNeeded;
    for (const FString &Code : UniqueCodes)
        ShapesNeeded.AddUnique(SignBenchmark::ShapeForCode(Code));

    if (UniqueCodes.IsEmpty() || ShapesNeeded.IsEmpty())
    {
        OnComplete(false);
        return;
    }

    const int32 TotalRequests = ShapesNeeded.Num() + UniqueCodes.Num();
    Arm(TotalRequests, [this, CodeSequence, UniqueCodes, OnComplete]()
    {
        TAssetsReady = FPlatformTime::Seconds();
        Stats.PrepareSeconds = TAssetsReady - TStart;
        const double SpawnStart = FPlatformTime::Seconds();

        // One ISM per unique code, sharing that code's shape's single mesh asset.
        for (const FString &Code : UniqueCodes)
        {
            const FString ShapeKey = SignBenchmark::ShapeName(SignBenchmark::ShapeForCode(Code));

            TObjectPtr<UStaticMesh> *MeshPtr = MeshByShape.Find(ShapeKey);
            TObjectPtr<UTexture2D> *TexPtr = TexByCode.Find(Code);
            if (!MeshPtr || !*MeshPtr || !TexPtr || !*TexPtr)
            {
                UE_LOG(LogTemp, Warning, TEXT("SignBench PerTexture: missing mesh/texture for %s, skipping"), *Code);
                continue;
            }

            UInstancedStaticMeshComponent *ISM = NewObject<UInstancedStaticMeshComponent>(Root);
            ISM->SetStaticMesh(*MeshPtr);
            ISM->SetMobility(EComponentMobility::Static);
            ISM->SetupAttachment(Root->GetRootComponent());
            ISM->RegisterComponent();

            UMaterialInstanceDynamic *MID = UMaterialInstanceDynamic::Create(TexturedBaseMaterial, ISM);
            MID->SetTextureParameterValue(TEXT("SignTex"), *TexPtr);

            const int32 FaceSlot = SignBenchmark::FindSignFaceSlot(*MeshPtr);
            ISM->SetMaterial(FaceSlot, MID);

            ISMByCode.Add(Code, ISM);
            ++Stats.ComponentCount;
        }

        for (int32 i = 0; i < CodeSequence.Num(); ++i)
        {
            TObjectPtr<UInstancedStaticMeshComponent> *ISMPtr = ISMByCode.Find(CodeSequence[i]);
            if (!ISMPtr || !*ISMPtr)
                continue;

            (*ISMPtr)->AddInstance(SignBenchmark::ComputeGridTransform(i, Grid), false);
            ++Stats.InstanceCount;
        }

        for (const auto &Pair : ISMByCode)
        {
            if (Pair.Value)
                Pair.Value->MarkRenderStateDirty();
        }

        Stats.UniqueMeshCount = MeshByShape.Num();
        Stats.UniqueMaterialCount = ISMByCode.Num();
        Stats.UniqueTextureCount = TexByCode.Num();
        Stats.SpawnSeconds = FPlatformTime::Seconds() - SpawnStart;

        OnComplete(true);
    });

    for (ESignShape Shape : ShapesNeeded)
    {
        const FString ShapeKey = SignBenchmark::ShapeName(Shape);
        const FString Url = BaseUrl + TEXT("PerTexture/") + ShapeKey + TEXT(".glb");
        USignGlbRequestProxy::RequestMesh(this, Url, [this, ShapeKey](const FString &, UStaticMesh *Mesh)
        {
            if (Mesh)
                MeshByShape.Add(ShapeKey, Mesh);
            else
                UE_LOG(LogTemp, Warning, TEXT("SignBench PerTexture: failed to load mesh for shape %s"), *ShapeKey);
            Signal();
        });
    }

    for (const FString &Code : UniqueCodes)
    {
        const FString Url = BaseUrl + TEXT("PerTexture/") + Code + TEXT(".png");
        Assets()->RequestTexture(Url, [this, Code](UTexture2D *Tex)
        {
            if (Tex)
                TexByCode.Add(Code, Tex);
            else
                UE_LOG(LogTemp, Warning, TEXT("SignBench PerTexture: failed to load texture for code %s"), *Code);
            Signal();
        });
    }
}

void USignSpawnStrategy_PerTexture::Clear()
{
    MeshByShape.Reset();
    TexByCode.Reset();
    ISMByCode.Reset();
    Super::Clear();
}
