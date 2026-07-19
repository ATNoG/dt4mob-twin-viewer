#include "UI/InfoFieldRegistry.h"
#include "Kismet/GameplayStatics.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "Engine/LocalPlayer.h"

const FString UInfoFieldRegistry::SaveSlotName = TEXT("EntityInfoFields");

void UInfoFieldRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadSave();
}

// ── Public API ────────────────────────────────────────────────────────────────

TArray<FInfoField> UInfoFieldRegistry::GetFields(const FString& TypeKey) const
{
    if (SaveGame)
    {
        if (const FInfoFieldList* Saved = SaveGame->FieldsByType.Find(TypeKey))
            return Saved->Fields;
    }

    return GetDefaultFields(TypeKey);
}

void UInfoFieldRegistry::SetFields(const FString& TypeKey, const TArray<FInfoField>& Fields)
{
    if (!SaveGame)
        return;

    SaveGame->FieldsByType.FindOrAdd(TypeKey).Fields = Fields;
    WriteSave();
    OnInfoFieldsChanged.Broadcast(TypeKey);
}

TArray<FInfoField> UInfoFieldRegistry::GetDefaultFields(const FString& TypeKey) const
{
    if (ULocalPlayer* LP = GetLocalPlayer())
        if (UGameInstance* GI = LP->GetGameInstance())
            if (UDT4MOBEntityFactory* Factory = GI->GetSubsystem<UDT4MOBEntityFactory>())
                return Factory->GetExtensionForType(TypeKey)->GetDefaultInfoFields();

    return {};
}

void UInfoFieldRegistry::ResetToDefaults(const FString& TypeKey)
{
    if (!SaveGame)
        return;

    SaveGame->FieldsByType.Remove(TypeKey);
    WriteSave();
    OnInfoFieldsChanged.Broadcast(TypeKey);
}

// ── Persistence ───────────────────────────────────────────────────────────────

void UInfoFieldRegistry::LoadSave()
{
    if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
        SaveGame = Cast<UEntityInfoSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, 0));

    if (!SaveGame)
        SaveGame = Cast<UEntityInfoSaveGame>(UGameplayStatics::CreateSaveGameObject(UEntityInfoSaveGame::StaticClass()));
}

void UInfoFieldRegistry::WriteSave()
{
    if (SaveGame)
        UGameplayStatics::SaveGameToSlot(SaveGame, SaveSlotName, 0);
}
