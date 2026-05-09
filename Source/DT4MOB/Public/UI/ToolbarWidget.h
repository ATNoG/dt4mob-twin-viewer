#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "ToolbarWidget.generated.h"

class UPlacementManager;

/**
 * @brief HUD toolbar with tool buttons (e.g. place ignition point).
 *
 * Binds to UPlacementManager to reflect the active tool state.
 * The Blueprint layout must bind PlaceIgnitionPointButton.
 */
UCLASS()
class DT4MOB_API UToolbarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

protected:
    /** @brief Button that activates ignition-point placement mode. Must be bound in Blueprint. */
    UPROPERTY(meta = (BindWidget))
    UButton* PlaceIgnitionPointButton;

private:
    UPROPERTY()
    UPlacementManager* PlacementManager = nullptr;

    UFUNCTION()
    void HandlePlaceIgnitionPointClicked(); 

    UFUNCTION()
    void HandlePlacementModeChanged(bool bActive);
};
