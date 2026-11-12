/**
 * @file
 * @brief Declares UMeshVisualSettings, the project settings page for entity mesh rendering.
 * @ingroup entities
 */

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MeshVisualSettings.generated.h"

class UMaterialInterface;

/**
 * @brief Project settings page shown under Project Settings -> Game -> Mesh Visual Settings.
 *
 * Assign a translucent "ghost" material here to back the per-layer transparency
 * toggle in the Models tab (ATempUIActor::SetMeshLayerTranslucent). If it is left unset
 * the toggle has no visual effect and logs a warning. Turning the toggle off always
 * restores the layer's original materials.
 *
 * @ingroup entities
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mesh Visual Settings"))
class DT4MOB_API UMeshVisualSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** @brief Files the settings page under the "Game" category. */
    UMeshVisualSettings() { CategoryName = TEXT("Game"); }

    /**
     * @brief Translucent material put on every material slot of a mesh layer while its
     *        transparency toggle is on.
     *
     * Loaded synchronously whenever a layer is made translucent. Edited in Project Settings
     * and saved to DefaultGame.ini.
     */
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Meshes")
    TSoftObjectPtr<UMaterialInterface> GhostMaterial;
};
