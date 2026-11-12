/**
 * @file
 * @brief Declares AIgnitionPointGhostActor, the cursor preview shown while placing an ignition point.
 * @ingroup entities
 */

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CesiumGlobeAnchorComponent.h"
#include "IgnitionPointGhostActor.generated.h"

/**
 * @brief Preview sphere that follows the cursor while the user is placing an ignition point.
 *
 * UPlacementManager spawns it hidden when placement mode starts, moves it with
 * MoveToWorldPosition() whenever the cursor trace hits the ground, hides it with
 * SetGhostVisible() when the trace misses, and destroys it when placement ends.
 * The actor does not tick and has collision disabled, so it never blocks the cursor trace.
 *
 * @ingroup entities
 */
UCLASS()
class DT4MOB_API AIgnitionPointGhostActor : public AActor
{
    GENERATED_BODY()

public:
    /** @brief Creates the globe anchor, the scene root and a half-scale engine sphere with collision off. */
    AIgnitionPointGhostActor();

    /**
     * @brief Moves the ghost to a world-space position.
     *
     * Sets the actor location directly; no ground snapping happens here.
     *
     * @param WorldPos  Unreal world location, normally the cursor's ground hit point.
     */
    void MoveToWorldPosition(const FVector& WorldPos);

    /**
     * @brief Shows or hides the sphere mesh.
     * @param bVisible  True to show it. UPlacementManager passes false when the cursor trace misses.
     */
    void SetGhostVisible(bool bVisible);

private:
    /** @brief Cesium globe anchor so the ghost keeps its geographic position if the georeference origin moves. */
    UPROPERTY(VisibleAnywhere)
    UCesiumGlobeAnchorComponent* GlobeAnchor;

    /** @brief Root component; the sphere mesh is attached to it. */
    UPROPERTY(VisibleAnywhere)
    USceneComponent* SceneRoot;

    /** @brief Engine sphere mesh at 0.5 scale with BasicShapeMaterial and no collision. */
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* Mesh;
};
