#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MeshVisualSettings.generated.h"

class UMaterialInterface;

/**
 * @brief Project Settings → Game → Mesh Visual Settings.
 *
 * Assign a translucent "ghost" material here to back the per-layer transparency
 * toggle in the Models tab (ATempUIActor::SetMeshLayerTranslucent). Leave unset to
 * disable the toggle's visual effect — the layer's original materials are always
 * restored when toggled off regardless.
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mesh Visual Settings"))
class DT4MOB_API UMeshVisualSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UMeshVisualSettings() { CategoryName = TEXT("Game"); }

    /** Translucent material swapped onto a mesh layer when its transparency toggle is on. */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TSoftObjectPtr<UMaterialInterface> GhostMaterial;
};
