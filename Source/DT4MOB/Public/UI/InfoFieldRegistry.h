#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "UI/InfoFieldTypes.h"
#include "InfoFieldRegistry.generated.h"

/** Broadcast when a type's field list is changed so open Info tabs can refresh. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInfoFieldsChanged, const FString&, TypeKey);

/**
 * Persists and serves per-type Info tab field lists.
 * Loaded from SaveGame on init; saved whenever fields are changed.
 * Provides built-in defaults so the tab is populated on first run.
 */
UCLASS()
class DT4MOB_API UInfoFieldRegistry : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** Returns the field list for a type key. Falls back to defaults if never customised. */
    UFUNCTION(BlueprintCallable, Category = "InfoFields")
    TArray<FInfoField> GetFields(const FString& TypeKey) const;

    /** Overwrites the field list for a type key and saves to disk. */
    UFUNCTION(BlueprintCallable, Category = "InfoFields")
    void SetFields(const FString& TypeKey, const TArray<FInfoField>& Fields);

    /** Resets a type key back to its built-in defaults and saves. */
    UFUNCTION(BlueprintCallable, Category = "InfoFields")
    void ResetToDefaults(const FString& TypeKey);

    /** Returns the built-in default fields for a type key, ignoring any saved customisation. */
    TArray<FInfoField> GetDefaultFields(const FString& TypeKey) const;

    UPROPERTY(BlueprintAssignable)
    FOnInfoFieldsChanged OnInfoFieldsChanged;

private:
    static const FString SaveSlotName;

    UPROPERTY()
    UEntityInfoSaveGame* SaveGame = nullptr;

    void LoadSave();
    void WriteSave();
};
