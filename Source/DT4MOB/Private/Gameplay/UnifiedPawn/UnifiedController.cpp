/** @file UnifiedController.cpp
 *  @brief Implementation of AUnifiedController. All logic documentation is in the header.
 */
#include "Gameplay/UnifiedPawn/UnifiedController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Gameplay/UnifiedPawn/UnifiedPawn.h"
#include "UI/RootHUDWidget.h"
#include "GameFramework/PlayerController.h"
#include "Managers/SelectionManager.h"
#include "Managers/PlacementManager.h"
#include "Services/DittoService.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "EntityStructs/IgnitionPointStruct.h"
#include "JsonObjectConverter.h"
#include "CesiumGeoreference.h"
#include "Misc/Guid.h"

void AUnifiedController::ApplyGameInputMode(ECameraMode NewMode)
{
    UE_LOG(LogTemp, Warning, TEXT("Applying game input mode: %d"), static_cast<int32>(NewMode));
    if (NewMode == ECameraMode::RTS)
    {
        // RTS always uses mouse interaction
        FInputModeGameAndUI InputMode;
        SetInputMode(InputMode);
        bShowMouseCursor = true;

        // Leaving FreeFly interaction state
        bFreeFlyMouseUnlocked = false;
    }
    else
    {
        // FreeFly defaults to locked mouse-look unless explicitly unlocked
        if (bFreeFlyMouseUnlocked)
        {
            FInputModeGameAndUI InputMode;
            SetInputMode(InputMode);
            bShowMouseCursor = true;
        }
        else
        {
            FInputModeGameOnly InputMode;
            SetInputMode(InputMode);
            bShowMouseCursor = false;
        }
    }
}

void AUnifiedController::BeginPlay()
{
    Super::BeginPlay();

    if (ULocalPlayer *LP = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem *Subsystem =
                LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            if (MappingContext)
            {
                Subsystem->AddMappingContext(MappingContext, 0);
            }
        }

        SelectionManager = LP->GetSubsystem<USelectionManager>();
        PlacementManager = LP->GetSubsystem<UPlacementManager>();
    }

    if (AUnifiedPawn *Cam = Cast<AUnifiedPawn>(GetPawn()))
    {
        ApplyGameInputMode(Cam->GetCameraMode());
    }

    if (HUDWidgetClass)
    {
        UUserWidget *HUD = CreateWidget<UUserWidget>(this, HUDWidgetClass);
        if (HUD)
        {
            HUD->AddToViewport();
        }
    }
}

void AUnifiedController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateHover();
}
void AUnifiedController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent *EIC =
            CastChecked<UEnhancedInputComponent>(InputComponent))
    {
        if (MoveAction)
        {
            EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AUnifiedController::Move);
            EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &AUnifiedController::Move);
            EIC->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AUnifiedController::Move);
        }

        if (LookAction)
        {
            EIC->BindAction(LookAction, ETriggerEvent::Triggered, this,
                            &AUnifiedController::Look);
        }

        if (ZoomAction)
        {
            EIC->BindAction(ZoomAction, ETriggerEvent::Triggered, this,
                            &AUnifiedController::Zoom);
        }
        if (LeftClickAction)
        {
            EIC->BindAction(LeftClickAction, ETriggerEvent::Started, this,
                            &AUnifiedController::LeftClick);
        }
        if (VerticalMoveAction)
        {
            EIC->BindAction(VerticalMoveAction, ETriggerEvent::Triggered, this, &AUnifiedController::VerticalMove);
            EIC->BindAction(VerticalMoveAction, ETriggerEvent::Completed, this, &AUnifiedController::VerticalMove);
            EIC->BindAction(VerticalMoveAction, ETriggerEvent::Canceled, this, &AUnifiedController::VerticalMove);
        }
        if (ToggleMouseUnlockAction)
        {
            EIC->BindAction(ToggleMouseUnlockAction, ETriggerEvent::Started, this, &AUnifiedController::ToggleFreeFlyMouseUnlock);
        }
    }
}

void AUnifiedController::Move(const FInputActionValue& Value)
{
    if (bMovementInputSuppressed) return;
    if (AUnifiedPawn* ControlledPawn = Cast<AUnifiedPawn>(GetPawn()))
        ControlledPawn->HandleMove(Value);
}


void AUnifiedController::Look(const FInputActionValue &Value)
{
    if (bMovementInputSuppressed) return;
    if (AUnifiedPawn *Cam = Cast<AUnifiedPawn>(GetPawn()))
    {
        if (Cam->GetCameraMode() == ECameraMode::FreeFly && bFreeFlyMouseUnlocked)
        {
            return;
        }

        Cam->HandleLook(Value);
    }
}

void AUnifiedController::Zoom(const FInputActionValue &Value)
{
    if (bMovementInputSuppressed) return;
    if (AUnifiedPawn *ControlledPawn = Cast<AUnifiedPawn>(GetPawn()))
    {
        ControlledPawn->HandleZoom(Value);
    }
}

void AUnifiedController::LeftClick(const FInputActionValue &Value)
{
    AUnifiedPawn *CamPawn = Cast<AUnifiedPawn>(GetPawn());
    if (!CamPawn)
        return;

    const ECameraMode Mode = CamPawn->GetCameraMode();
    const bool bCanUseCursorInteraction =
        (Mode == ECameraMode::RTS) ||
        (Mode == ECameraMode::FreeFly && bFreeFlyMouseUnlocked);

    if (!bCanUseCursorInteraction)
        return;

    // --- Placement mode: confirm entity placement ---
    if (PlacementManager && PlacementManager->IsPlacing())
    {
        if (!bLastTerrainHitValid)
            return;

        const FVector WorldPos = LastTerrainHit;
        PlacementManager->ConfirmPlacement(WorldPos);

        ACesiumGeoreference* GeoRef = ACesiumGeoreference::GetDefaultGeoreference(GetWorld());
        if (!GeoRef)
            return;

        const FVector LLH = GeoRef->TransformUnrealPositionToLongitudeLatitudeHeight(WorldPos);
        const double Lon = LLH.X;
        const double Lat = LLH.Y;

        const FString TypeKey = PlacementManager->GetSelectedTypeKey();
        const FString Guid = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens).ToLower();
        TSharedPtr<FJsonObject> Body = MakeShared<FJsonObject>();
        FString ThingId;

        if (TypeKey.IsEmpty() || TypeKey.StartsWith(TEXT("fire")))
        {
            FIgnitionPointData IgnitionPoint;
            IgnitionPoint.thingId = TEXT("fire:") + Guid;
            IgnitionPoint.attributes.fire_ignition.lon = Lon;
            IgnitionPoint.attributes.fire_ignition.lat = Lat;
            FJsonObjectConverter::UStructToJsonObject(FIgnitionPointData::StaticStruct(), &IgnitionPoint, Body.ToSharedRef(), 0, 0);
            ThingId = IgnitionPoint.thingId;
        }
        else
        {
            // Generic entity: build minimal JSON with thingId and flat lat/lon attributes
            ThingId = TypeKey + TEXT(":") + Guid;
            Body->SetStringField(TEXT("thingId"), ThingId);
            Body->SetStringField(TEXT("policyId"), TEXT("dt4mob:default"));

            TSharedPtr<FJsonObject> Attributes = MakeShared<FJsonObject>();
            Attributes->SetNumberField(TEXT("latitude"), Lat);
            Attributes->SetNumberField(TEXT("longitude"), Lon);
            Body->SetObjectField(TEXT("attributes"), Attributes);
        }

        if (UGameInstance* GI = GetGameInstance())
        {
            if (UDittoService* Ditto = GI->GetSubsystem<UDittoService>())
            {
                if (UDT4MOBEntityFactory* Factory = GI->GetSubsystem<UDT4MOBEntityFactory>())
                {
                    Factory->SpawnTempUIActor(GetWorld(), Body);
                }

                Ditto->PutThing(ThingId, Body,
                    [ThingId](bool bSuccess)
                    {
                        UE_LOG(LogTemp, Log, TEXT("Entity PUT [%s] %s"),
                               *ThingId, bSuccess ? TEXT("OK") : TEXT("FAILED"));
                    });
            }
        }
        return;
    }

    // --- Normal selection ---
    if (!SelectionManager)
        return;

    FHitResult Hit;
    if (TraceFromCursor(Hit))
        SelectionManager->SetSelectedActor(Hit.GetActor());
    else
        SelectionManager->SetSelectedActor(nullptr);
}

void AUnifiedController::VerticalMove(const FInputActionValue &Value)
{
    if (bMovementInputSuppressed) return;
    if (AUnifiedPawn *ControlledPawn = Cast<AUnifiedPawn>(GetPawn()))
    {
        ControlledPawn->HandleVerticalMove(Value);
    }
}

void AUnifiedController::ToggleCameraMode()
{
    if (AUnifiedPawn *ControlledPawn = Cast<AUnifiedPawn>(GetPawn()))
    {
        const ECameraMode NewMode =
            (ControlledPawn->GetCameraMode() == ECameraMode::RTS)
                ? ECameraMode::FreeFly
                : ECameraMode::RTS;

        ControlledPawn->SetCameraMode(NewMode);

        // Reset FreeFly mouse unlock when changing modes
        if (NewMode != ECameraMode::FreeFly)
        {
            bFreeFlyMouseUnlocked = false;
        }

        ApplyGameInputMode(NewMode);
    }
}

void AUnifiedController::ToggleFreeFlyMouseUnlock()
{
    if (AUnifiedPawn *CamPawn = Cast<AUnifiedPawn>(GetPawn()))
    {
        if (CamPawn->GetCameraMode() != ECameraMode::FreeFly)
        {
            return;
        }
    }

    SetFreeFlyMouseUnlocked(!bFreeFlyMouseUnlocked);
}

void AUnifiedController::SetFreeFlyMouseUnlocked(bool bUnlocked)
{
    AUnifiedPawn *Cam = Cast<AUnifiedPawn>(GetPawn());
    if (!Cam || Cam->GetCameraMode() != ECameraMode::FreeFly)
    {
        return;
    }

    bFreeFlyMouseUnlocked = bUnlocked;

    if (bFreeFlyMouseUnlocked)
    {
        FInputModeGameAndUI InputMode;
        SetInputMode(InputMode);
        bShowMouseCursor = true;
    }
    else
    {
        FInputModeGameOnly InputMode;
        SetInputMode(InputMode);
        bShowMouseCursor = false;
    }
}

bool AUnifiedController::TraceFromCursor(FHitResult &OutHit) const
{
    FVector WorldLocation;
    FVector WorldDirection;

    if (!DeprojectMousePositionToWorld(WorldLocation, WorldDirection))
    {
        return false;
    }

    const float TraceDistance = 500000000.f;
    const FVector TraceEnd = WorldLocation + (WorldDirection * TraceDistance);

    FCollisionQueryParams Params;
    if (const APawn *P = GetPawn())
    {
        Params.AddIgnoredActor(P);
    }

    FCollisionObjectQueryParams ObjectParams(ECC_WorldDynamic);
    return GetWorld()->LineTraceSingleByObjectType(
        OutHit,
        WorldLocation,
        TraceEnd,
        ObjectParams,
        Params);
}

void AUnifiedController::UpdateHover()
{
    AUnifiedPawn *CamPawn = Cast<AUnifiedPawn>(GetPawn());
    if (!CamPawn)
    {
        if (SelectionManager) SelectionManager->SetHoveredActor(nullptr);
        return;
    }

    const ECameraMode Mode = CamPawn->GetCameraMode();
    const bool bCanUseCursorInteraction =
        (Mode == ECameraMode::RTS) ||
        (Mode == ECameraMode::FreeFly && bFreeFlyMouseUnlocked);

    if (!bCanUseCursorInteraction)
    {
        if (SelectionManager) SelectionManager->SetHoveredActor(nullptr);
        if (PlacementManager) PlacementManager->UpdateGhostPosition(FVector::ZeroVector, false);
        return;
    }

    // Terrain trace for placement ghost (WorldStatic)
    if (PlacementManager && PlacementManager->IsPlacing())
    {
        FVector WorldLoc, WorldDir;
        bLastTerrainHitValid = false;
        if (DeprojectMousePositionToWorld(WorldLoc, WorldDir))
        {
            FHitResult TerrainHit;
            FCollisionQueryParams Params;
            if (const APawn* P = GetPawn()) Params.AddIgnoredActor(P);
            const bool bHit = GetWorld()->LineTraceSingleByObjectType(
                TerrainHit, WorldLoc, WorldLoc + WorldDir * 500000000.f,
                FCollisionObjectQueryParams(ECC_WorldStatic), Params);

            if (bHit)
            {
                LastTerrainHit = TerrainHit.ImpactPoint;
                bLastTerrainHitValid = true;
            }
            PlacementManager->UpdateGhostPosition(LastTerrainHit, bHit);
        }
        return; // don't update selection hover while placing
    }

    // Normal entity hover
    if (!SelectionManager)
        return;

    FHitResult Hit;
    if (TraceFromCursor(Hit))
        SelectionManager->SetHoveredActor(Hit.GetActor());
    else
        SelectionManager->SetHoveredActor(nullptr);
}

void AUnifiedController::SetMovementInputSuppressed(bool bSuppressed)
{
    bMovementInputSuppressed = bSuppressed;
}