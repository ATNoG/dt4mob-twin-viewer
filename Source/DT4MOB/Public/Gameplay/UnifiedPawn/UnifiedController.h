/**
 * @file
 * @brief Declares AUnifiedController, the main-scene player controller for AUnifiedPawn.
 * @ingroup gameplay
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "CameraMode.h"
#include "UnifiedController.generated.h"

class UInputMappingContext;
class UInputAction;
class USelectionManager;
class UPlacementManager;

/**
 * @brief Player controller for the main scene: Enhanced Input bindings, input mode per camera mode,
 *        cursor hover/selection and entity placement clicks.
 *
 * On BeginPlay it:
 *  - Registers the Enhanced Input mapping context.
 *  - Caches the USelectionManager and UPlacementManager local player subsystems.
 *  - Returns to the login level if UBackendConnectionService is not authenticated; otherwise
 *    creates the HUD widget.
 *
 * Each tick UpdateHover() traces from the cursor and updates the SelectionManager hover state,
 * or moves the placement ghost while placement mode is active.
 *
 * Cursor interaction is available in RTS mode, and in FreeFly mode only after the mouse has
 * been unlocked with ToggleFreeFlyMouseUnlock().
 *
 * @ingroup gameplay
 */
UCLASS()
class DT4MOB_API AUnifiedController : public APlayerController
{
    GENERATED_BODY()

public:
    /**
     * @brief Applies the Unreal input mode and cursor visibility for the given camera mode.
     *
     * RTS always shows the cursor, uses GameAndUI input and clears bFreeFlyMouseUnlocked.
     * FreeFly uses GameOnly with a hidden cursor unless the mouse is unlocked, in which case it
     * uses GameAndUI with a visible cursor.
     *
     * @param NewMode The camera mode that just became active.
     */
    void ApplyGameInputMode(ECameraMode NewMode);

    /**
     * @brief Switches the controlled AUnifiedPawn between RTS and FreeFly camera modes.
     *
     * Clears bFreeFlyMouseUnlocked when switching to RTS and re-applies the input mode. Does
     * nothing if the pawn is not an AUnifiedPawn. Called from the toolbar (UToolbarWidget).
     */
    void ToggleCameraMode();

    /** @brief Widget class that CreateHUD() instantiates and adds to the viewport. Set in the controller Blueprint. */
    UPROPERTY(EditAnywhere, Category = "UI")
    TSubclassOf<class UUserWidget> HUDWidgetClass;

    /**
     * @brief Login level opened by Logout(), and by BeginPlay when there is no valid Ditto session.
     *
     * Normally the player only reaches the main scene through
     * ALoginPlayerController::GoToMainScene(), which has already authenticated, so the
     * BeginPlay fallback only matters when the level is opened directly (e.g. in the editor).
     */
    UPROPERTY(EditAnywhere, Category = "UI")
    FName LoginLevelName = TEXT("/Game/Scenes/Login/Login");

    /**
     * @brief Flips the FreeFly mouse-unlock state through SetFreeFlyMouseUnlocked().
     *
     * Bound to ToggleMouseUnlockAction. Has no effect outside FreeFly mode.
     */
    void ToggleFreeFlyMouseUnlock();

    /**
     * @brief Sets the FreeFly mouse-unlock state and updates input mode and cursor visibility.
     *
     * Does nothing unless the controlled pawn is an AUnifiedPawn in FreeFly mode.
     *
     * @param bUnlocked True to show the cursor and switch to GameAndUI input; false for GameOnly with a hidden cursor.
     */
    void SetFreeFlyMouseUnlocked(bool bUnlocked);

    /**
     * @brief Returns whether the cursor is currently unlocked in FreeFly mode.
     * @return True if the mouse is unlocked; always false after switching to RTS.
     */
    bool IsFreeFlyMouseUnlocked() const { return bFreeFlyMouseUnlocked; }

    /**
     * @brief Temporarily blocks camera input, for example while a UI window is being dragged.
     *
     * While suppressed, Move, Look, Zoom and VerticalMove input is dropped before it reaches the
     * pawn. Left clicks and hover tracing are not affected.
     *
     * @param bSuppressed True to block camera input, false to allow it again.
     */
    UFUNCTION(BlueprintCallable)
    void SetMovementInputSuppressed(bool bSuppressed);

    /**
     * @brief Logs out and returns to the login level.
     *
     * Calls UBackendConnectionService::Logout() to clear the in-memory session, wipes stored
     * credentials through UCredentialStoreService::ClearCredentials(), then opens
     * LoginLevelName. Reloading the level is how all spawned entities, tiles and UI are torn down.
     */
    UFUNCTION(BlueprintCallable)
    void Logout();

    /**
     * @brief Returns whether camera input is currently suppressed.
     * @return True while SetMovementInputSuppressed(true) is in effect.
     */
    bool IsMovementInputSuppressed() const { return bMovementInputSuppressed; }

    /**
     * @brief Temporary debug console command that downloads a .glb through UGlbModelService and logs the result.
     *
     * Use from the in-game console (~) as:
     *     TestGlbDownload https://your-bucket.s3.amazonaws.com/path/model.glb
     * The outcome is logged by OnTestGlbDownloadResult(). Remove once the S3 download path is
     * confirmed working.
     *
     * @param Url  Address of the .glb file to request. An empty string only logs the usage line.
     */
    UFUNCTION(Exec)
    void TestGlbDownload(const FString &Url);

protected:
    /**
     * @brief Registers the input mapping context, caches the manager subsystems, applies the input mode and creates the HUD.
     *
     * Assumes UBackendConnectionService is already authenticated, because the player normally
     * arrives here through ALoginPlayerController::GoToMainScene(), which logs in first. If that
     * does not hold (for example the level was opened directly in the editor), it opens
     * LoginLevelName instead of creating the HUD.
     */
    virtual void BeginPlay() override;

    /**
     * @brief Binds each configured Enhanced Input action to its handler.
     *
     * Move and VerticalMove are also bound to Completed and Canceled so the zero value on
     * release reaches the pawn. LeftClick and ToggleMouseUnlock fire on Started. Actions that
     * are not set are skipped.
     */
    virtual void SetupInputComponent() override;

    /**
     * @brief Runs UpdateHover() every frame.
     * @param DeltaSeconds Time elapsed since the last frame, in seconds.
     */
    virtual void Tick(float DeltaSeconds) override;

private:
    /**
     * @brief Forwards move input to AUnifiedPawn::HandleMove() unless camera input is suppressed.
     * @param Value FVector2D input action value.
     */
    void Move(const FInputActionValue &Value);

    /**
     * @brief Forwards look input to AUnifiedPawn::HandleLook().
     *
     * Dropped while camera input is suppressed, and in FreeFly mode while the mouse is unlocked.
     *
     * @param Value FVector2D input action value.
     */
    void Look(const FInputActionValue &Value);

    /**
     * @brief Forwards zoom input to AUnifiedPawn::HandleZoom() unless camera input is suppressed.
     * @param Value Scalar input action value.
     */
    void Zoom(const FInputActionValue &Value);

    /**
     * @brief Handles a left click: confirms a placement in placement mode, otherwise selects the entity under the cursor.
     *
     * Ignored unless cursor interaction is available (RTS, or FreeFly with the mouse unlocked).
     * In placement mode the click needs a valid LastTerrainHit. It ends placement mode, converts
     * the hit point to latitude/longitude, builds the thing JSON with the type extension's
     * BuildPlacementJson() for the selected type key ("fire:" when none is selected), spawns the
     * actor locally and sends the thing to Ditto with UBackendConnectionService::PutThing(). The
     * PUT result is only logged. Outside placement mode the clicked actor (or nullptr) is passed
     * to USelectionManager::SetSelectedActor().
     *
     * @param Value Input action value (not read).
     */
    void LeftClick(const FInputActionValue &Value);

    /**
     * @brief Forwards vertical-move input to AUnifiedPawn::HandleVerticalMove() unless camera input is suppressed.
     * @param Value Scalar input action value.
     */
    void VerticalMove(const FInputActionValue &Value);

    /**
     * @brief Line-traces from the cursor into the world against WorldDynamic objects.
     *
     * The trace is 500,000,000 cm (5,000 km) long and ignores the controlled pawn.
     *
     * @param OutHit  Receives the first blocking hit, if any.
     * @return True if something was hit; false if the mouse position could not be deprojected or nothing was hit.
     */
    bool TraceFromCursor(FHitResult &OutHit) const;

    /**
     * @brief Updates hover state, or the placement ghost while placement mode is active.
     *
     * Clears the hover (and hides the ghost) when cursor interaction is not available. In
     * placement mode it traces against WorldStatic geometry, stores the result in
     * LastTerrainHit/bLastTerrainHitValid and moves the ghost there, without touching hover.
     * Otherwise it passes the TraceFromCursor() hit (or nullptr) to
     * USelectionManager::SetHoveredActor().
     */
    void UpdateHover();

    /**
     * @brief Logs the outcome of TestGlbDownload() with the elapsed time. Dynamic delegates can only bind UFUNCTIONs, not lambdas.
     * @param Mesh  Mesh built from the downloaded file, or nullptr if the download or parse failed.
     */
    UFUNCTION()
    void OnTestGlbDownloadResult(class UStaticMesh *Mesh);

    /** @brief Creates HUDWidgetClass, adds it to the viewport and re-applies the input mode. Does nothing if HUDWidgetClass is unset. */
    void CreateHUD();

    /** @brief URL of the in-flight TestGlbDownload() request, read by OnTestGlbDownloadResult(). */
    FString TestGlbDownloadUrl;

    /** @brief FPlatformTime::Seconds() when the in-flight TestGlbDownload() request started. */
    double TestGlbDownloadStartTime = 0.0;

    /** @brief Enhanced Input mapping context added at priority 0 on BeginPlay. Set in the controller Blueprint defaults. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputMappingContext *MappingContext;

    /** @brief Input action for 2D panning/movement (FVector2D). Set in the controller Blueprint defaults. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction *MoveAction;

    /** @brief Input action for camera look/rotation (FVector2D). Set in the controller Blueprint defaults. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction *LookAction;

    /** @brief Input action for zoom, e.g. the mouse wheel (float). Set in the controller Blueprint defaults. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction *ZoomAction;

    /** @brief Input action for selection and placement clicks. Set in the controller Blueprint defaults. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction *LeftClickAction;

    /** @brief Input action for FreeFly movement along globe up/down (float). Set in the controller Blueprint defaults. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction *VerticalMoveAction;

    /** @brief Input action that toggles the FreeFly mouse unlock. Set in the controller Blueprint defaults. */
    UPROPERTY(EditDefaultsOnly, Category = "Input")
    UInputAction *ToggleMouseUnlockAction;

    /** @brief Local player USelectionManager subsystem, cached on BeginPlay. */
    UPROPERTY()
    USelectionManager *SelectionManager = nullptr;

    /** @brief Local player UPlacementManager subsystem, cached on BeginPlay. */
    UPROPERTY()
    UPlacementManager *PlacementManager = nullptr;

    /** @brief Impact point of the last WorldStatic cursor trace in placement mode; LeftClick() places the entity here. */
    FVector LastTerrainHit = FVector::ZeroVector;

    /** @brief True if the last placement-mode terrain trace hit something. LeftClick() ignores placement clicks while false. */
    bool bLastTerrainHitValid = false;

    /** @brief True when the cursor is unlocked in FreeFly mode, enabling cursor interaction. */
    bool bFreeFlyMouseUnlocked = false;

    /** @brief Set by SetMovementInputSuppressed(); while true, camera input handlers return early. */
    bool bMovementInputSuppressed = false;
};
