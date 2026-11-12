// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief Adapter that lets a TFunction callback receive a mesh from UGlbModelService::RequestMesh.
 * @ingroup signbenchmark
 */

#pragma once

#include "CoreMinimal.h"
#include "SignGlbRequestProxy.generated.h"

class UStaticMesh;

/**
 * @brief One-shot adapter between UGlbModelService::RequestMesh's dynamic delegate (which a lambda cannot bind to) and a plain TFunction callback.
 *
 * Each request creates one proxy UObject, adds it to the root set so it survives until the
 * dynamic delegate fires, then removes it from the root set and calls the TFunction. Used by the
 * benchmark strategies and by USinalizacaoModelComponent.
 * @ingroup signbenchmark
 */
UCLASS()
class DT4MOB_API USignGlbRequestProxy : public UObject
{
    GENERATED_BODY()

public:
    /**
     * @brief Requests the GLB at Url from UGlbModelService and calls OnLoaded exactly once with the result.
     *
     * On a session cache hit in the service, OnLoaded runs before this function returns.
     * Otherwise the service sends a GET (conditional when it has an ETag) and loads its disk copy
     * on a 304.
     *
     * @param Owner     Object used to find the world and game instance; also the outer of the proxy.
     * @param Url       Full URL of the GLB file.
     * @param OnLoaded  Receives Url and the loaded mesh, or nullptr if loading failed or no
     *                  UGlbModelService is available (in that case it runs immediately).
     */
    static void RequestMesh(UObject *Owner, const FString &Url, TFunction<void(const FString &Url, UStaticMesh *Mesh)> OnLoaded);

private:
    /**
     * @brief Dynamic delegate target: removes the proxy from the root set and forwards Mesh to the stored callback.
     * @param Mesh  Loaded mesh, or nullptr on failure.
     */
    UFUNCTION()
    void HandleMeshLoaded(UStaticMesh *Mesh);

    /** @brief URL of the request, passed back to the callback. */
    FString Url;

    /** @brief Callback from RequestMesh(). Moved out and cleared before it is called. */
    TFunction<void(const FString &, UStaticMesh *)> Callback;
};
