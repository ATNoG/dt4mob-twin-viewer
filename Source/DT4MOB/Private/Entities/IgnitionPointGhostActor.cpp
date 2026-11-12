/**
 * @file
 * @brief Implements AIgnitionPointGhostActor. Member documentation is in the header.
 * @ingroup entities
 */
#include "Entities/IgnitionPointGhostActor.h"

AIgnitionPointGhostActor::AIgnitionPointGhostActor()
{
    PrimaryActorTick.bCanEverTick = false;

    GlobeAnchor = CreateDefaultSubobject<UCesiumGlobeAnchorComponent>(TEXT("GlobeAnchor"));

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (Sphere.Succeeded())
    {
        Mesh->SetStaticMesh(Sphere.Object);
    }

    Mesh->SetWorldScale3D(FVector(0.5f));
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Meant to make the ghost read as a preview rather than a placed entity. Note that
    // BasicShapeMaterial is the engine's plain opaque shape material, not a translucent one.
    static ConstructorHelpers::FObjectFinder<UMaterial> TranslucentMat(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (TranslucentMat.Succeeded())
    {
        Mesh->SetMaterial(0, TranslucentMat.Object);
    }

    SetActorEnableCollision(false);
}

void AIgnitionPointGhostActor::MoveToWorldPosition(const FVector& WorldPos)
{
    SetActorLocation(WorldPos);
}

void AIgnitionPointGhostActor::SetGhostVisible(bool bVisible)
{
    Mesh->SetVisibility(bVisible);
}
