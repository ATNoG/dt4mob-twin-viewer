#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CesiumGlobeAnchorComponent.h"
#include "IgnitionPointGhostActor.generated.h"

/**
 * @brief Translucent preview actor shown while the user is in ignition-point placement mode.
 *
 * Follows the terrain cursor position every tick via MoveToWorldPosition().
 * Uses a sphere mesh with a translucent material to signal "not yet placed".
 */
UCLASS()
class DT4MOB_API AIgnitionPointGhostActor : public AActor
{
    GENERATED_BODY()

public:
    AIgnitionPointGhostActor();

    /** @brief Move the ghost to a world-space position (snapped via GlobeAnchor). */
    void MoveToWorldPosition(const FVector& WorldPos);

    /** @brief Show or hide the ghost (hidden when cursor misses terrain). */
    void SetGhostVisible(bool bVisible);

private:
    UPROPERTY(VisibleAnywhere)
    UCesiumGlobeAnchorComponent* GlobeAnchor;

    UPROPERTY(VisibleAnywhere)
    USceneComponent* SceneRoot;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh;
};
