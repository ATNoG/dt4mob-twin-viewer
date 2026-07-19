#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "glTFRuntimeFunctionLibrary.h"
#include "glTFRuntimeAsset.h"
#include "GlbModelService.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGlbMeshLoaded, UStaticMesh *, Mesh);

/** @brief One named sub-mesh extracted from a multi-mesh GLB, with its node-relative transform. */
USTRUCT(BlueprintType)
struct FGlbMeshLayer
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString Name;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UStaticMesh> Mesh = nullptr;

    UPROPERTY(BlueprintReadOnly)
    FTransform Transform = FTransform::Identity;
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGlbMeshLayersLoaded, const TArray<FGlbMeshLayer> &, Layers);

/** @brief GC-safe wrapper so TMap<FString, ...> doesn't need a raw nested-container UPROPERTY. */
USTRUCT()
struct FGlbMeshLayerArray
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FGlbMeshLayer> Layers;
};

/** @brief Persistent per-URL entry written to disk. */
struct FGlbCacheEntry
{
    FString ETag;
    FString LocalPath;
};

/**
 * @brief GameInstance subsystem that fetches GLB models over HTTP with ETag-based
 * disk caching.
 *
 * First request: GET → save body to disk → store ETag in etags.json → load mesh(es).
 * Subsequent requests (same session): served instantly from the in-memory cache.
 * Subsequent requests (new session): GET with If-None-Match → 304 → load from disk.
 * Content changed: server returns 200 with new ETag → overwrite file → reload.
 *
 * Multiple actors may request the same URL concurrently; only one HTTP request is
 * made and all callbacks fire together on completion.
 */
UCLASS()
class DT4MOB_API UGlbModelService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;

    /**
     * @brief Request a single UStaticMesh for Url, with every node/mesh in the GLB
     * merged together. Fires Callback immediately if cached, otherwise starts
     * (or joins an in-flight) HTTP request.
     */
    void RequestMesh(const FString &Url, const FOnGlbMeshLoaded &Callback);

    /**
     * @brief Request Url as a set of individually-loaded mesh layers — one UStaticMesh
     * per glTF node that has a mesh, each with its own node-relative FTransform, instead
     * of being flattened into a single merged mesh. Use this for entity model loading so a
     * GLB bundling several independent sub-models (e.g. per-timestep fire simulation frames)
     * can be shown, hidden, or styled independently at runtime.
     */
    void RequestMeshLayers(const FString &Url, const FOnGlbMeshLayersLoaded &Callback);

private:
    /** @brief Session-only URL → merged mesh map. Populated after a successful RequestMesh load. */
    UPROPERTY()
    TMap<FString, UStaticMesh *> MeshCache;

    /** @brief Session-only URL → per-node mesh layers. Populated after a successful RequestMeshLayers load. */
    UPROPERTY()
    TMap<FString, FGlbMeshLayerArray> MeshLayersCache;

    /** @brief Persisted URL → {ETag, LocalPath}. Loaded from / saved to disk. */
    TMap<FString, FGlbCacheEntry> ETagCache;

    /** @brief Callbacks waiting on each in-flight URL (RequestMesh path). */
    TMap<FString, TArray<FOnGlbMeshLoaded>> PendingCallbacks;

    /** @brief Callbacks waiting on each in-flight URL (RequestMeshLayers path). */
    TMap<FString, TArray<FOnGlbMeshLayersLoaded>> PendingLayerCallbacks;

    /** @brief Downloads Url if needed (ETag-conditional) and invokes OnReady with the local file path, or an empty string on failure. */
    void EnsureFileOnDisk(const FString &Url, TFunction<void(const FString &FilePath)> OnReady);

    void LoadMeshFromFile(const FString &Url, const FString &FilePath);
    void LoadMeshLayersFromFile(const FString &Url, const FString &FilePath);

    void FireCallbacks(const FString &Url, UStaticMesh *Mesh);
    void FireLayerCallbacks(const FString &Url, const TArray<FGlbMeshLayer> &Layers);

    FString GetCacheFilePath(const FString &Url) const;
    void LoadETagCache();
    void SaveETagCache();

    static FString ETagCachePath();
    static FString CacheDir();
};
