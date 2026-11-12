/**
 * @file
 * @brief Declares UInfoFieldRegistry, which stores the Info tab field selection per entity type.
 * @ingroup ui
 */
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
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UInfoFieldRegistry : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    /**
     * @brief Loads the saved selection, or creates an empty save game if there is none.
     * @param Collection Subsystem collection.
     */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /**
     * @brief Returns the field list for a type key. Falls back to defaults if never customised.
     * @param TypeKey Entity type key.
     * @return The fields to show, in order.
     */
    UFUNCTION(BlueprintCallable, Category = "InfoFields")
    TArray<FInfoField> GetFields(const FString& TypeKey) const;

    /**
     * @brief Overwrites the field list for a type key, saves to disk and broadcasts OnInfoFieldsChanged.
     * @param TypeKey Entity type key.
     * @param Fields  New field list.
     */
    UFUNCTION(BlueprintCallable, Category = "InfoFields")
    void SetFields(const FString& TypeKey, const TArray<FInfoField>& Fields);

    /**
     * @brief Resets a type key back to its built-in defaults, saves and broadcasts OnInfoFieldsChanged.
     * @param TypeKey Entity type key.
     */
    UFUNCTION(BlueprintCallable, Category = "InfoFields")
    void ResetToDefaults(const FString& TypeKey);

    /**
     * @brief Returns the built-in default fields for a type key, ignoring any saved customisation.
     * The defaults come from the type's UEntityTypeExtension::GetDefaultInfoFields().
     * @param TypeKey Entity type key.
     * @return The default fields, or an empty array if the factory is not available.
     */
    TArray<FInfoField> GetDefaultFields(const FString& TypeKey) const;

    /** @brief Fired with the type key after its field list was changed or reset. */
    UPROPERTY(BlueprintAssignable)
    FOnInfoFieldsChanged OnInfoFieldsChanged;

private:
    /** @brief Save slot name ("EntityInfoFields"), user index 0. */
    static const FString SaveSlotName;

    /** @brief In-memory copy of the saved selection. */
    UPROPERTY()
    UEntityInfoSaveGame* SaveGame = nullptr;

    /** @brief Loads SaveGame from the slot, or creates a new one. */
    void LoadSave();

    /** @brief Writes SaveGame to the slot. */
    void WriteSave();
};
