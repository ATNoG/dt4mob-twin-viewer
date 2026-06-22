#include "UI/InfoFieldRegistry.h"
#include "Kismet/GameplayStatics.h"

const FString UInfoFieldRegistry::SaveSlotName = TEXT("EntityInfoFields");
TMap<FString, TArray<FInfoField>> UInfoFieldRegistry::Defaults;

void UInfoFieldRegistry::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Defaults = BuildDefaults();
    LoadSave();
}

// ── Public API ────────────────────────────────────────────────────────────────

TArray<FInfoField> UInfoFieldRegistry::GetFields(const FString& TypeKey) const
{
    if (SaveGame)
    {
        if (const FInfoFieldList* Saved = SaveGame->FieldsByType.Find(TypeKey))
            if (!Saved->Fields.IsEmpty())
                return Saved->Fields;
    }

    if (const TArray<FInfoField>* Default = Defaults.Find(TypeKey))
        return *Default;

    return {};
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
    if (const TArray<FInfoField>* Found = Defaults.Find(TypeKey))
        return *Found;
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

// ── Defaults ──────────────────────────────────────────────────────────────────

TMap<FString, TArray<FInfoField>> UInfoFieldRegistry::BuildDefaults()
{
    TMap<FString, TArray<FInfoField>> D;

    D.Add("traci", {
        { "Speed",        "features.state.properties.speed"     },
        { "Heading",      "features.state.properties.angle"     },
        { "Latitude",     "features.state.properties.latitude"  },
        { "Longitude",    "features.state.properties.longitude" },
        { "Acceleration", "features.state.properties.accel"     },
        { "TTL",          "attributes.time_to_live"             },
    });

    D.Add("meteo", {
        { "Station",       "attributes.location_name"                                  },
        { "Latitude",      "attributes.location.latitude"                              },
        { "Longitude",     "attributes.location.longitude"                             },
        { "Temperature",   "features.meteorology.properties.temperature"               },
        { "Humidity",      "features.meteorology.properties.humidity"                  },
        { "Wind Speed",    "features.meteorology.properties.wind_intensity"            },
        { "Wind Dir.",     "features.meteorology.properties.wind_direction"            },
        { "Pressure",      "features.meteorology.properties.pressure"                  },
        { "Precipitation", "features.meteorology.properties.accumulated_precipitation" },
        { "Last Reading",  "features.meteorology.properties.time"                      },
    });

    D.Add("fire:", {
        { "State",     "attributes.state"             },
        { "Latitude",  "attributes.fire_ignition.lat" },
        { "Longitude", "attributes.fire_ignition.lon" },
    });

    D.Add("barrier", {
        { "Latitude",  "attributes.location.latitude"  },
        { "Longitude", "attributes.location.longitude" },
    });

    D.Add("sign", {
        { "Latitude",  "attributes.location.latitude"  },
        { "Longitude", "attributes.location.longitude" },
    });

    D.Add("muro-talude", {
        { "Latitude",  "attributes.location.latitude"  },
        { "Longitude", "attributes.location.longitude" },
    });

    D.Add("geo-asset", {
        { "Matricula",  "attributes.matricula"           },
        { "Location",   "attributes.localizacao"         },
        { "Latitude",   "attributes.latitude"            },
        { "Longitude",  "attributes.longitude"           },
        { "Condition",  "attributes.indiceCondicaoAtual" },
        { "Height",     "attributes.altura"              },
        { "Length",     "attributes.extensao"            },
    });

    D.Add(".instrument.", {
        { "Matricula",  "attributes.matricula"                   },
        { "Type",       "attributes.tipoInstrumento.name"        },
        { "Latitude",   "attributes.coordinates.latitude"        },
        { "Longitude",  "attributes.coordinates.longitude"       },
    });

    return D;
}
