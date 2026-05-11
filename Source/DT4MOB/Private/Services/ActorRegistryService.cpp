#include "Services/ActorRegistryService.h"
#include "Entities/TempUIActor.h"
#include "Engine/GameInstance.h"

UActorRegistryService *UActorRegistryService::Get(const UObject *WorldContext)
{
    if (!WorldContext) return nullptr;
    UWorld *World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull);
    if (!World) return nullptr;
    UGameInstance *GI = World->GetGameInstance();
    if (!GI) return nullptr;
    return GI->GetSubsystem<UActorRegistryService>();
}

void UActorRegistryService::RegisterActor(const FString &ThingId, ATempUIActor *Actor)
{
    if (!ThingId.IsEmpty() && Actor)
        Registry.Add(ThingId, Actor);
}

void UActorRegistryService::UnregisterActor(const FString &ThingId)
{
    Registry.Remove(ThingId);
}

ATempUIActor *UActorRegistryService::FindActor(const FString &ThingId) const
{
    const ATempUIActor *const *Found = Registry.Find(ThingId);
    return Found ? const_cast<ATempUIActor *>(*Found) : nullptr;
}

TArray<ATempUIActor *> UActorRegistryService::FindActorsWithPrefix(const FString &Prefix) const
{
    TArray<ATempUIActor *> Result;
    for (const auto &Pair : Registry)
    {
        if (Pair.Key.StartsWith(Prefix))
            Result.Add(Pair.Value);
    }
    return Result;
}
