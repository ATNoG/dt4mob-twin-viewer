#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "CameraMode.h"
#include "UnifiedController.generated.h"

class UInputMappingContext;
class UInputAction;
class USelectionManager;

/**
 * @brief Player controller that manages Enhanced Input bindings, camera mode toggling,
 *        and cursor-based entity selection/hover for AUnifiedPawn.
 *
 * On BeginPlay it:
 *  - Registers the Enhanced Input mapping context.
 *  - Caches a pointer to USelectionManager.
 *  - Creates and adds the HUD widget to the viewport.
 *
 * Each tick UpdateHover() performs a cursor trace and updates the SelectionManager hover state.
 *
 * In FreeFly mode the mouse can be unlocked via ToggleFreeFlyMouseUnlock() to allow
 * cursor-based interaction without leaving FreeFly.
 */
UCLASS()
class DT4MOB_API AUnifiedController : public APlayerController
{
    GENERATED_BODY()

public:
    /**
     * @brief Applies the appropriate Unreal input mode and cursor visibility for the given camera mode.
     *
     * RTS always shows the cursor and enables GameAndUI input.
     * FreeFly defaults to GameOnly (hidden cursor) unless the mouse is explicitly unlocked.
     *
     * @param NewMode The camera mode that just became active.
     */
    void ApplyGameInputMode(ECameraMode NewMode);

    /**
     * @brief Toggles the pawn between RTS and FreeFly camera modes.
     *
     * Also resets bFreeFlyMouseUnlocked when leaving FreeFly.
     */
    void ToggleCameraMode();

    /** @brief Blueprint-assignable class used to create and display the HUD widget on BeginPlay. */
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<class UUserWidget> HUDWidgetClass;

    /**
     * @brief Toggles the FreeFly mouse-unlock state (cursor visible, cursor interaction allowed).
     *
     * Only takes effect when the pawn is in FreeFly mode.
     */
    void ToggleFreeFlyMouseUnlock();

    /**
     * @brief Explicitly sets the FreeFly mouse-unlock state.
     *
     * Only takes effect when the pawn is currently in FreeFly mode.
     * Updates input mode and cursor visibility accordingly.
     *
     * @param bUnlocked True to show the cursor and switch to GameAndUI input; false for GameOnly.
     */
    void SetFreeFlyMouseUnlocked(bool bUnlocked);

    /**
     * @brief Returns whether the FreeFly mouse is currently unlocked.
     * @return True if the mouse is unlocked in FreeFly mode.
     */
    bool IsFreeFlyMouseUnlocked() const { return bFreeFlyMouseUnlocked; }

protected:
    /** @brief Initialises Enhanced Input, caches SelectionManager, and spawns the HUD. */
    virtual void BeginPlay() override;

    /** @brief Binds all Enhanced Input actions to their handler functions. */
    virtual void SetupInputComponent() override;

    /**
     * @brief Each tick performs a cursor hover trace and updates SelectionManager.
     * @param DeltaSeconds Time elapsed since the last frame.
     */
    virtual void Tick(float DeltaSeconds) override;

private:
    /**
     * @brief Forwards move input to the controlled pawn.
     * @param Value FVector2D input action value.
     */
    void Move(const FInputActionValue &Value);

    /**
     * @brief Forwards look input to the controlled pawn (suppressed in FreeFly when mouse is unlocked).
     * @param Value FVector2D input action value.
     */
    void Look(const FInputActionValue &Value);

    /**
     * @brief Forwards zoom input to the controlled pawn.
     * @param Value Scalar input action value.
     */
    void Zoom(const FInputActionValue &Value);

    /**
     * @brief Performs a cursor trace on left-click and updates the SelectionManager selection.
     * @param Value Input action value (unused beyond trigger detection).
     */
    void LeftClick(const FInputActionValue &Value);

    /**
     * @brief Forwards vertical-move input to the controlled pawn.
     * @param Value Scalar input action value.
     */
    void VerticalMove(const FInputActionValue &Value);

    /**
     * @brief Performs a world-space line trace from the cursor position along the custom entity channel.
     * @param OutHit  Receives the first blocking hit if any.
     * @return True if a hit was recorded.
     */
    bool TraceFromCursor(FHitResult &OutHit) const;

    /** @brief Each tick traces from the cursor and forwards the result to SelectionManager::SetHoveredActor. */
    void UpdateHover();

    /** @brief Enhanced Input mapping context to register on BeginPlay. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputMappingContext *MappingContext;

    /** @brief Input action for 2D panning/movement. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction *MoveAction;

    /** @brief Input action for camera look/rotation. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction *LookAction;

    /** @brief Input action for zoom (e.g. mouse wheel). */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction *ZoomAction;

    /** @brief Input action for entity selection via left mouse button. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction *LeftClickAction;

    /** @brief Input action for vertical movement (globe-up/down). */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction *VerticalMoveAction;

    /** @brief Input action that toggles FreeFly mouse unlock. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction *ToggleMouseUnlockAction;

    /** @brief Cached pointer to the LocalPlayer SelectionManager subsystem. */
    UPROPERTY()
    USelectionManager *SelectionManager = nullptr;

    /** @brief True when the cursor is unlocked in FreeFly mode, enabling cursor interaction. */
    bool bFreeFlyMouseUnlocked = false;
};
