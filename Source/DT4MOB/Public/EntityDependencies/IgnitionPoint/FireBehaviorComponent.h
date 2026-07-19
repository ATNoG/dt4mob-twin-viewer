// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EntityDependencies/EntityBehaviorComponent.h"
#include "Services/GlbModelService.h"
#include "FireBehaviorComponent.generated.h"

/**
 * @brief Per-actor behavior for "fire:" (ignition point) entities: loads the cone/simulation
 *        multi-model GLBs, fetches and parses the cone/perimeter-step GeoJSON, and supplies the
 *        precise exclusion-polygon shape once parsed (see GetExclusionPolygonPoints).
 *
 * Attached generically by ATempUIActor::Initialize() via UIgnitionPointExtension::GetBehaviorComponentClass().
 */
UCLASS()
class DT4MOB_API UFireBehaviorComponent : public UEntityBehaviorComponent
{
    GENERATED_BODY()

public:
    virtual void OnEntityInitialized() override;
    virtual void OnEntityDataChanged() override;
    virtual bool HandlesOwnModelLoading() const override { return true; }
    virtual bool GetExclusionPolygonPoints(TArray<FVector2D>& OutPoints) const override;

private:
    /** @brief Loads any URLs from attributes.polygon not already requested — "Cone" layer group
     *  first, "Simulation" layer group second — then fetches perimeter GeoJSON. */
    void TryLoadFireGlbModels();

    /** @brief Triggers HTTP GETs for any cone / perimeter-step GeoJSON URLs not yet fetched. */
    void TryFetchFirePerimeters();

    /** @brief Callback for the cone (polygon[0]) GLB — split into the "Cone" layer group. */
    UFUNCTION()
    void OnConeGlbLayersLoaded(const TArray<FGlbMeshLayer>& GlbLayers);

    /** @brief Callback for the simulation (polygon[1]) GLB — split into the "Simulation" layer group. */
    UFUNCTION()
    void OnSimulationGlbLayersLoaded(const TArray<FGlbMeshLayer>& GlbLayers);

    /** @brief Parses the outer ring of a GeoJSON Polygon/Feature/FeatureCollection.
     *  Returns FVector2D points where X = latitude, Y = longitude. */
    static TArray<FVector2D> ParseGeoJsonOuterRing(const FString& JsonStr);

    /** @brief Combines every feature's ring in a cone-horizon GeoJSON FeatureCollection (one
     *  section per time horizon) into a single convex hull. X = latitude, Y = longitude. */
    static TArray<FVector2D> ParseConeGeoJsonHull(const FString& JsonStr);

    /** @brief URLs already requested from GlbModelService; prevents duplicate loads. */
    TSet<FString> FireGlbLoadedUrls;

    /** @brief URL of the last cone GeoJSON requested; guards against re-fetches. */
    FString FetchedConeGeoJsonUrl;

    /** @brief Step URLs already requested; guards against re-fetches on repeated patches. */
    TSet<FString> FetchedPerimeterStepUrls;

    /** @brief Parsed outer ring from the cone horizon GeoJSON (X=lat, Y=lon). */
    TArray<FVector2D> ParsedConePerimeterPoints;

    /** @brief Parsed outer ring per time step from the perimeter step GeoJSONs (X=lat, Y=lon). */
    TArray<TArray<FVector2D>> ParsedPerimeterSteps;

    /** @brief True once all perimeter step GeoJSONs have been fetched and parsed. */
    bool bSimulationStepsReady = false;
};
