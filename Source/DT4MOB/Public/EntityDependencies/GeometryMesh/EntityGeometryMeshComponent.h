// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityBehaviorComponent.h"
#include "EntityGeometryMeshComponent.generated.h"

class UProceduralMeshComponent;

/**
 * @brief Per-actor behavior that turns an entity's attributes.geometry point array (plus
 *        shape_Length/shape_Area, when present) into a visible mesh: a filled footprint for
 *        closed shapes (shape_Area present/non-zero, or the ring is explicitly closed), or a
 *        thin ribbon tracing the path for open polylines.
 *
 * Reflection-based, unlike UFireBehaviorComponent — it has to work across the many differently
 * typed geometry point structs (FInfPtObrasArtePoint, FInfPtDrenagemLinearPoint, ...) without
 * per-type code, so it can't cast to one concrete struct the way Fire does.
 *
 * Attached generically by ATempUIActor::Initialize() via
 * UEntityTypeExtension::GetBehaviorComponentClass() for any entity type that opts in.
 */
UCLASS()
class DT4MOB_API UEntityGeometryMeshComponent : public UEntityBehaviorComponent
{
    GENERATED_BODY()

public:
    virtual void OnEntityInitialized() override;
    virtual void OnEntityDataChanged() override;

    /** @brief Debug toggle: when false, RebuildMesh() is a no-op — the entity still spawns and
     *  places correctly, it just gets no procedural mesh. Parking the feature (flat-height mesh
     *  doesn't follow terrain slope yet) without ripping the component out. Flip back to true
     *  when resuming work on it. */
    UPROPERTY(EditDefaultsOnly, Category = "Debug")
    bool bMeshGenerationEnabled = false;

private:
    /** @brief One point extracted from the entity's geometry array, in decimal degrees. */
    struct FGeoPoint
    {
        double Latitude = 0.0;
        double Longitude = 0.0;
    };

    /** @brief Re-reads geometry from the owner's StructInstance and rebuilds the mesh section.
     *  No-op (and leaves any previously built mesh alone) if the struct has no geometry array. */
    void RebuildMesh();

    /** @brief Reflects over StructInstance for a `geometry` array of {latitude,longitude} points
     *  and shape_Length/shape_Area scalars (any casing). Returns false if no geometry array field
     *  exists on the struct at all. */
    static bool ExtractGeometry(const TSharedPtr<FStructOnScope>& StructInstance, TArray<FGeoPoint>& OutPoints, double& OutShapeLength, double& OutShapeArea);

    /** @brief Builds a flat, filled triangulated mesh for a closed ring via FGeomTools2D::TriangulatePoly. */
    static void BuildFilledMesh(const TArray<FVector>& LocalPoints, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles, TArray<FVector>& OutNormals, TArray<FVector2D>& OutUVs);

    /** @brief Builds a thin extruded ribbon strip tracing an open polyline. */
    static void BuildRibbonMesh(const TArray<FVector>& LocalPoints, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles, TArray<FVector>& OutNormals, TArray<FVector2D>& OutUVs);

    /** @brief Lazily created the first time RebuildMesh() finds valid geometry. Never exists on actors without one. */
    UPROPERTY()
    UProceduralMeshComponent* MeshComponent = nullptr;

    /** @brief Half-width (cm) of the ribbon extruded for open polylines. */
    static constexpr float RibbonHalfWidthCm = 25.f;

    /** @brief Height (cm) the mesh is drawn above its geometry points' ground level. */
    static constexpr float MeshHeightCm = 5.f;
};
