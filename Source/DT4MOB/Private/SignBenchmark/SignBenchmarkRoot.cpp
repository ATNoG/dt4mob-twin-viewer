#include "SignBenchmark/SignBenchmarkRoot.h"

ASignBenchmarkRoot::ASignBenchmarkRoot()
{
    PrimaryActorTick.bCanEverTick = false;

    USceneComponent *Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Root->SetMobility(EComponentMobility::Static);
    SetRootComponent(Root);
}
