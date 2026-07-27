// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SignGlbRequestProxy.generated.h"

class UStaticMesh;

/**
 * @brief One-shot adapter between UGlbModelService::RequestMesh's dynamic delegate
 * (which lambdas can't bind to) and a plain TFunction callback.
 *
 * Creates a rooted, short-lived UObject per request so it survives until the
 * dynamic delegate fires, then unroots itself and invokes the TFunction.
 */
UCLASS()
class DT4MOB_API USignGlbRequestProxy : public UObject
{
    GENERATED_BODY()

public:
    /** @brief Fetches Url via UGlbModelService (session/disk cache aware) and invokes OnLoaded(Url, Mesh) exactly once, synchronously if already cached. */
    static void RequestMesh(UObject *Owner, const FString &Url, TFunction<void(const FString &Url, UStaticMesh *Mesh)> OnLoaded);

private:
    UFUNCTION()
    void HandleMeshLoaded(UStaticMesh *Mesh);

    FString Url;
    TFunction<void(const FString &, UStaticMesh *)> Callback;
};
