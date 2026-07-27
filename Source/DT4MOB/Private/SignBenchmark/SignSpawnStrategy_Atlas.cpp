#include "SignBenchmark/SignSpawnStrategy_Atlas.h"
#include "SignBenchmark/SignBenchmarkRoot.h"
#include "SignBenchmark/SignGlbRequestProxy.h"
#include "SignBenchmark/SignAssetService.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"

void USignSpawnStrategy_Atlas::Execute(const TArray<FString> &CodeSequence, TFunction<void(bool)> OnComplete)
{
    MeshByShape.Reset();
    AtlasTexByShape.Reset();
    AtlasTableByShape.Reset();
    ISMByShape.Reset();

    if (!AtlasBaseMaterial)
    {
        UE_LOG(LogTemp, Error, TEXT("SignBench AtlasMaterial: AtlasBaseMaterial not set"));
        OnComplete(false);
        return;
    }

    TArray<ESignShape> ShapesNeeded;
    for (const FString &Code : CodeSequence)
        ShapesNeeded.AddUnique(SignBenchmark::ShapeForCode(Code));

    if (ShapesNeeded.IsEmpty())
    {
        OnComplete(false);
        return;
    }

    // 3 requests per shape: mesh, atlas texture, atlas JSON.
    Arm(ShapesNeeded.Num() * 3, [this, CodeSequence, ShapesNeeded, OnComplete]()
    {
        TAssetsReady = FPlatformTime::Seconds();
        Stats.PrepareSeconds = TAssetsReady - TStart;
        const double SpawnStart = FPlatformTime::Seconds();

        for (ESignShape Shape : ShapesNeeded)
        {
            const FString ShapeKey = SignBenchmark::ShapeName(Shape);

            TObjectPtr<UStaticMesh> *MeshPtr = MeshByShape.Find(ShapeKey);
            TObjectPtr<UTexture2D> *AtlasTexPtr = AtlasTexByShape.Find(ShapeKey);
            FSignAtlasTable *TablePtr = AtlasTableByShape.Find(ShapeKey);
            if (!MeshPtr || !*MeshPtr || !AtlasTexPtr || !*AtlasTexPtr || !TablePtr)
            {
                UE_LOG(LogTemp, Warning, TEXT("SignBench AtlasMaterial: missing assets for shape %s, skipping"), *ShapeKey);
                continue;
            }

            UInstancedStaticMeshComponent *ISM = NewObject<UInstancedStaticMeshComponent>(Root);
            ISM->SetStaticMesh(*MeshPtr);
            ISM->SetMobility(EComponentMobility::Static);
            ISM->SetNumCustomDataFloats(4); // must happen before any AddInstance
            ISM->SetupAttachment(Root->GetRootComponent());
            ISM->RegisterComponent();

            UMaterialInstanceDynamic *MID = UMaterialInstanceDynamic::Create(AtlasBaseMaterial, ISM);
            MID->SetTextureParameterValue(TEXT("AtlasTex"), *AtlasTexPtr);

            const int32 FaceSlot = SignBenchmark::FindSignFaceSlot(*MeshPtr);
            ISM->SetMaterial(FaceSlot, MID);

            ISMByShape.Add(ShapeKey, ISM);
            ++Stats.ComponentCount;
        }

        for (int32 i = 0; i < CodeSequence.Num(); ++i)
        {
            const FString &Code = CodeSequence[i];
            const FString ShapeKey = SignBenchmark::ShapeName(SignBenchmark::ShapeForCode(Code));

            TObjectPtr<UInstancedStaticMeshComponent> *ISMPtr = ISMByShape.Find(ShapeKey);
            FSignAtlasTable *TablePtr = AtlasTableByShape.Find(ShapeKey);
            if (!ISMPtr || !*ISMPtr || !TablePtr)
                continue;

            const int32 InstanceIndex = (*ISMPtr)->AddInstance(SignBenchmark::ComputeGridTransform(i, Grid), false);

            const FSignAtlasUV *UV = TablePtr->Cells.Find(Code);
            FSignAtlasUV Fallback; // {0,0,1,1} - shows the whole atlas, visibly wrong rather than silently wrong
            if (!UV)
            {
                UE_LOG(LogTemp, Warning, TEXT("SignBench AtlasMaterial: no atlas UV entry for code %s, falling back to full atlas"), *Code);
                UV = &Fallback;
            }

            const float Data[4] = {UV->UOffset, UV->VOffset, UV->UScale, UV->VScale};
            (*ISMPtr)->SetCustomData(InstanceIndex, TArrayView<const float>(Data, 4), false);

            ++Stats.InstanceCount;
        }

        // One dirty per ISM, not per instance — dirtying render state 850 times is a
        // measurable and misleading cost that has nothing to do with what's measured.
        for (const auto &Pair : ISMByShape)
        {
            if (Pair.Value)
                Pair.Value->MarkRenderStateDirty();
        }

        Stats.UniqueMeshCount = MeshByShape.Num();
        Stats.UniqueMaterialCount = ISMByShape.Num();
        Stats.UniqueTextureCount = AtlasTexByShape.Num();
        Stats.SpawnSeconds = FPlatformTime::Seconds() - SpawnStart;

        OnComplete(true);
    });

    for (ESignShape Shape : ShapesNeeded)
    {
        const FString ShapeKey = SignBenchmark::ShapeName(Shape);

        const FString MeshUrl = BaseUrl + TEXT("AtlasMaterial/") + ShapeKey + TEXT(".glb");
        USignGlbRequestProxy::RequestMesh(this, MeshUrl, [this, ShapeKey](const FString &, UStaticMesh *Mesh)
        {
            if (Mesh)
                MeshByShape.Add(ShapeKey, Mesh);
            else
                UE_LOG(LogTemp, Warning, TEXT("SignBench AtlasMaterial: failed to load mesh for shape %s"), *ShapeKey);
            Signal();
        });

        const FString AtlasTexUrl = BaseUrl + TEXT("AtlasMaterial/") + ShapeKey + TEXT("_atlas.png");
        Assets()->RequestTexture(AtlasTexUrl, [this, ShapeKey](UTexture2D *Tex)
        {
            if (Tex)
                AtlasTexByShape.Add(ShapeKey, Tex);
            else
                UE_LOG(LogTemp, Warning, TEXT("SignBench AtlasMaterial: failed to load atlas texture for shape %s"), *ShapeKey);
            Signal();
        });

        const FString AtlasJsonUrl = BaseUrl + TEXT("AtlasMaterial/") + ShapeKey + TEXT("_atlas.json");
        Assets()->RequestAtlas(AtlasJsonUrl, [this, ShapeKey](const FSignAtlasTable *Table)
        {
            if (Table)
                AtlasTableByShape.Add(ShapeKey, *Table);
            else
                UE_LOG(LogTemp, Warning, TEXT("SignBench AtlasMaterial: failed to load atlas JSON for shape %s"), *ShapeKey);
            Signal();
        });
    }
}

void USignSpawnStrategy_Atlas::Clear()
{
    MeshByShape.Reset();
    AtlasTexByShape.Reset();
    AtlasTableByShape.Reset();
    ISMByShape.Reset();
    Super::Clear();
}
