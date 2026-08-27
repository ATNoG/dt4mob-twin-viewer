// Fill out your copyright notice in the Description page of Project Settings.

/** @file SinalizacaoModelComponent.cpp
 *  @brief Implementation of USinalizacaoModelComponent. All logic documentation is in the header.
 */
#include "EntityDependencies/Sinalizacao/SinalizacaoModelComponent.h"
#include "Entities/TempUIActor.h"
#include "SignBenchmark/SignGlbRequestProxy.h"
#include "SignBenchmark/SignAssetService.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/GameInstance.h"

const FString &USinalizacaoModelComponent::BaseUrl()
{
    static const FString Url = TEXT("https://dt4mob.av.it.pt/s3/dt4mob-public/modelTest/");
    return Url;
}

UMaterialInterface *USinalizacaoModelComponent::AtlasBaseMaterial()
{
    static TWeakObjectPtr<UMaterialInterface> Cached;
    if (Cached.IsValid())
        return Cached.Get();

    UMaterialInterface *Mat = LoadObject<UMaterialInterface>(
        nullptr, TEXT("/Game/Materials/SignBenchmark/M_SignAtlas.M_SignAtlas"));
    Cached = Mat;
    return Mat;
}

void USinalizacaoModelComponent::OnEntityInitialized()
{
    OnEntityDataChanged();
}

void USinalizacaoModelComponent::OnEntityDataChanged()
{
    ATempUIActor *Owner = GetOwnerEntity();
    if (!Owner || !Owner->RawJson.IsValid())
        return;

    const TSharedPtr<FJsonObject> *AttribObj = nullptr;
    if (!Owner->RawJson->TryGetObjectField(TEXT("attributes"), AttribObj) || !AttribObj)
        return;

    FString Code;
    if (!(*AttribObj)->TryGetStringField(TEXT("Code"), Code) || Code.IsEmpty())
        return;

    if (Code == CurrentCode)
        return;

    CurrentCode = Code;

    const bool bKnownCode = SignBenchmark::CircleCodes().Contains(Code) || SignBenchmark::TriangleCodes().Contains(Code);
    if (!bKnownCode)
    {
        // Not one of the benchmark's 17 codes with real atlas art — fall back to the generic
        // attributes.polygon GLB path (ATempUIActor::TryLoadGlbModel(), called right after this
        // in the same Initialize()/patch-handler pass) instead of showing a wrong full-atlas face.
        bUsingAtlasModel = false;
        if (FaceISM)
        {
            FaceISM->DestroyComponent();
            FaceISM = nullptr;
        }
        Owner->StaticMeshComponent->SetVisibility(true);

        UE_LOG(LogTemp, Log, TEXT("SinalizacaoModelComponent [%s]: not in the atlas catalog, falling back to attributes.polygon GLB"), *Code);
        return;
    }

    bUsingAtlasModel = true;
    LoadForCode(Code);
}

void USinalizacaoModelComponent::LoadForCode(const FString &Code)
{
    UMaterialInterface *BaseMat = AtlasBaseMaterial();
    if (!BaseMat)
    {
        UE_LOG(LogTemp, Error, TEXT("SinalizacaoModelComponent [%s]: M_SignAtlas not found at /Game/Materials/SignBenchmark/M_SignAtlas"),
            *Code);
        return;
    }

    const ESignShape Shape = SignBenchmark::ShapeForCode(Code);
    const FString ShapeKey = SignBenchmark::ShapeName(Shape);

    bMeshReady = false;
    bTexReady = false;
    bAtlasReady = false;
    ShapeMesh = nullptr;
    AtlasTex = nullptr;
    AtlasTable = FSignAtlasTable();

    const int32 Generation = ++RequestGeneration;

    const FString MeshUrl = BaseUrl() + TEXT("AtlasMaterial/") + ShapeKey + TEXT(".glb");
    USignGlbRequestProxy::RequestMesh(this, MeshUrl, [this, Generation](const FString &Url, UStaticMesh *Mesh)
    {
        if (Generation != RequestGeneration)
            return; // superseded by a newer Code
        ShapeMesh = Mesh;
        bMeshReady = true;
        if (!Mesh)
            UE_LOG(LogTemp, Warning, TEXT("SinalizacaoModelComponent: failed to load shape mesh from '%s'"), *Url);
        ApplyWhenReady();
    });

    ATempUIActor *Owner = GetOwnerEntity();
    UGameInstance *GI = Owner ? Owner->GetGameInstance() : nullptr;
    USignAssetService *Assets = GI ? GI->GetSubsystem<USignAssetService>() : nullptr;
    if (!Assets)
    {
        UE_LOG(LogTemp, Error, TEXT("SinalizacaoModelComponent [%s]: SignAssetService unavailable"), *Code);
        return;
    }

    const FString AtlasTexUrl = BaseUrl() + TEXT("AtlasMaterial/") + ShapeKey + TEXT("_atlas.png");
    Assets->RequestTexture(AtlasTexUrl, [this, Generation](UTexture2D *Tex)
    {
        if (Generation != RequestGeneration)
            return;
        AtlasTex = Tex;
        bTexReady = true;
        if (!Tex)
            UE_LOG(LogTemp, Warning, TEXT("SinalizacaoModelComponent: failed to load atlas texture"));
        ApplyWhenReady();
    });

    const FString AtlasJsonUrl = BaseUrl() + TEXT("AtlasMaterial/") + ShapeKey + TEXT("_atlas.json");
    Assets->RequestAtlas(AtlasJsonUrl, [this, Generation](const FSignAtlasTable *Table)
    {
        if (Generation != RequestGeneration)
            return;
        if (Table)
            AtlasTable = *Table;
        bAtlasReady = true;
        if (!Table)
            UE_LOG(LogTemp, Warning, TEXT("SinalizacaoModelComponent: failed to load atlas JSON"));
        ApplyWhenReady();
    });
}

void USinalizacaoModelComponent::ApplyWhenReady()
{
    if (!bMeshReady || !bTexReady || !bAtlasReady)
        return;

    ATempUIActor *Owner = GetOwnerEntity();
    if (!Owner || !ShapeMesh || !AtlasTex)
        return;

    if (FaceISM)
    {
        FaceISM->DestroyComponent();
        FaceISM = nullptr;
    }

    // Single-instance ISM, not the generic mesh-layer path — M_SignAtlas's UV remap reads
    // per-instance custom data, which only a real instanced component can feed (see class
    // comment). SetupAttachment to the actor's own root so it moves/rebases with it normally.
    FaceISM = NewObject<UInstancedStaticMeshComponent>(Owner, TEXT("SignFaceISM"));
    FaceISM->SetStaticMesh(ShapeMesh);
    FaceISM->SetMobility(EComponentMobility::Movable);
    FaceISM->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FaceISM->SetNumCustomDataFloats(4); // must happen before AddInstance
    FaceISM->SetupAttachment(Owner->GetRootComponent());
    FaceISM->RegisterComponent();

    Owner->StaticMeshComponent->SetVisibility(false);

    UMaterialInstanceDynamic *MID = UMaterialInstanceDynamic::Create(AtlasBaseMaterial(), FaceISM);
    MID->SetTextureParameterValue(TEXT("AtlasTex"), AtlasTex);

    const int32 FaceSlot = SignBenchmark::FindSignFaceSlot(ShapeMesh);
    FaceISM->SetMaterial(FaceSlot, MID);

    const int32 InstanceIndex = FaceISM->AddInstance(FTransform::Identity, false);

    const FSignAtlasUV *UV = AtlasTable.Cells.Find(CurrentCode);
    FSignAtlasUV Fallback; // {0,0,1,1} - shows the whole atlas, visibly wrong rather than silently wrong
    if (!UV)
    {
        UE_LOG(LogTemp, Warning, TEXT("SinalizacaoModelComponent: no atlas UV entry for code %s, falling back to full atlas"), *CurrentCode);
        UV = &Fallback;
    }

    const float Data[4] = {UV->UOffset, UV->VOffset, UV->UScale, UV->VScale};
    FaceISM->SetCustomData(InstanceIndex, TArrayView<const float>(Data, 4), false);
    FaceISM->MarkRenderStateDirty();

    UE_LOG(LogTemp, Log, TEXT("SinalizacaoModelComponent [%s]: applied real sign model + atlas UV"), *CurrentCode);
}
