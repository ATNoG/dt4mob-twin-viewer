#include "SignBenchmark/SignGlbRequestProxy.h"
#include "Services/GlbModelService.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void USignGlbRequestProxy::RequestMesh(UObject *Owner, const FString &Url, TFunction<void(const FString &, UStaticMesh *)> OnLoaded)
{
    UWorld *World = Owner ? Owner->GetWorld() : nullptr;
    UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    UGlbModelService *Service = GameInstance ? GameInstance->GetSubsystem<UGlbModelService>() : nullptr;

    if (!Service)
    {
        UE_LOG(LogTemp, Warning, TEXT("SignGlbRequestProxy: no GlbModelService available for %s"), *Url);
        OnLoaded(Url, nullptr);
        return;
    }

    USignGlbRequestProxy *Proxy = NewObject<USignGlbRequestProxy>(Owner);
    Proxy->Url = Url;
    Proxy->Callback = MoveTemp(OnLoaded);
    Proxy->AddToRoot();

    FOnGlbMeshLoaded Delegate;
    Delegate.BindDynamic(Proxy, &USignGlbRequestProxy::HandleMeshLoaded);
    Service->RequestMesh(Url, Delegate);
}

void USignGlbRequestProxy::HandleMeshLoaded(UStaticMesh *Mesh)
{
    TFunction<void(const FString &, UStaticMesh *)> LocalCallback = MoveTemp(Callback);
    const FString LocalUrl = Url;
    Callback = nullptr;

    RemoveFromRoot();

    if (LocalCallback)
        LocalCallback(LocalUrl, Mesh);
}
