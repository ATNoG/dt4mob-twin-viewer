/**
 * @file
 * @brief Declares FGlbMeshLayer, FGlbMeshLayerArray, FGlbCacheEntry and UGlbModelService.
 * @ingroup services
 */
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "glTFRuntimeFunctionLibrary.h"
#include "glTFRuntimeAsset.h"
#include "GlbModelService.generated.h"

/** @brief Callback for UGlbModelService::RequestMesh. @param Mesh Merged mesh, or null on failure. @ingroup services */
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGlbMeshLoaded, UStaticMesh *, Mesh);

/** @brief One named sub-mesh extracted from a multi-mesh GLB, with its node-relative transform. @ingroup services */
USTRUCT(BlueprintType)
struct FGlbMeshLayer
{
    GENERATED_BODY()

    /** @brief Name of the glTF node the mesh came from. */
    UPROPERTY(BlueprintReadOnly)
    FString Name;

    /** @brief Mesh built from the node. */
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UStaticMesh> Mesh = nullptr;

    /** @brief Node transform relative to the GLB root, in Unreal units. */
    UPROPERTY(BlueprintReadOnly)
    FTransform Transform = FTransform::Identity;
};

/** @brief Callback for UGlbModelService::RequestMeshLayers. @param Layers One mesh per GLB node, empty on failure. @ingroup services */
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGlbMeshLayersLoaded, const TArray<FGlbMeshLayer> &, Layers);

/** @brief GC-safe wrapper so TMap<FString, ...> doesn't need a raw nested-container UPROPERTY. @ingroup services */
USTRUCT()
struct FGlbMeshLayerArray
{
    GENERATED_BODY()

    /** @brief The wrapped layers. */
    UPROPERTY()
    TArray<FGlbMeshLayer> Layers;
};

/** @brief Persistent per-URL entry written to disk. @ingroup services */
struct FGlbCacheEntry
{
    FString ETag;      ///< ETag the server returned for the file.
    FString LocalPath; ///< Path of the cached GLB on disk.
};

/**
 * @brief GameInstance subsystem that fetches GLB models over HTTP with ETag-based
 * disk caching.
 *
 * First request: GET -> save body to disk -> store ETag in etags.json -> load mesh(es).
 * Subsequent requests (same session): served instantly from the in-memory cache.
 * Subsequent requests (new session): GET with If-None-Match -> 304 -> load from disk.
 * Content changed: server returns 200 with new ETag -> overwrite file -> reload.
 *
 * Multiple actors may request the same URL concurrently; only one HTTP request is
 * made and all callbacks fire together on completion.
 * @ingroup services
 */
UCLASS()
class DT4MOB_API UGlbModelService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /**
     * @brief Creates the cache directory (Saved/GlbCache) if needed and loads etags.json.
     * @param Collection Subsystem collection.
     */
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;

    /**
     * @brief Request a single UStaticMesh for Url, with every node/mesh in the GLB
     * merged together. Fires Callback immediately if cached, otherwise starts
     * (or joins an in-flight) HTTP request.
     * @param Url      URL of the GLB file.
     * @param Callback Called with the mesh, or with null if loading failed.
     */
    void RequestMesh(const FString &Url, const FOnGlbMeshLoaded &Callback);

    /**
     * @brief Request Url as a set of individually-loaded mesh layers: one UStaticMesh
     * per glTF node that has a mesh, each with its own node-relative FTransform, instead
     * of being flattened into a single merged mesh. Use this for entity model loading so a
     * GLB bundling several independent sub-models (e.g. per-timestep fire simulation frames)
     * can be shown, hidden, or styled independently at runtime.
     * @param Url      URL of the GLB file.
     * @param Callback Called with the layers, or with an empty array if loading failed.
     */
    void RequestMeshLayers(const FString &Url, const FOnGlbMeshLayersLoaded &Callback);

    /** @brief Testing/benchmark utility only: never called by production code. Empties the
     *  session-only MeshCache/MeshLayersCache so the next RequestMesh/RequestMeshLayers for an
     *  already-seen URL is forced back through EnsureFileOnDisk instead of returning instantly
     *  from memory. Also empties the in-memory ETagCache: EnsureFileOnDisk sends a conditional
     *  GET using whatever ETag it finds there, and on a 304 response it hands back
     *  Entry->LocalPath without checking the file still exists: if the caller deleted the
     *  on-disk cache directory but left this map alone, that path points at a file that no
     *  longer exists, the load silently fails, and the pending request never completes (this
     *  was an actual hang caught by testing, not a hypothetical). Deleting the on-disk cache
     *  directory alone clears neither of these in-memory maps. */
    void ClearSessionCache();

private:
    /** @brief Session-only URL -> merged mesh map. Populated after a successful RequestMesh load. */
    UPROPERTY()
    TMap<FString, UStaticMesh *> MeshCache;

    /** @brief Session-only URL -> per-node mesh layers. Populated after a successful RequestMeshLayers load. */
    UPROPERTY()
    TMap<FString, FGlbMeshLayerArray> MeshLayersCache;

    /** @brief Persisted URL -> {ETag, LocalPath}. Loaded from / saved to disk. */
    TMap<FString, FGlbCacheEntry> ETagCache;

    /** @brief Callbacks waiting on each in-flight URL (RequestMesh path). */
    TMap<FString, TArray<FOnGlbMeshLoaded>> PendingCallbacks;

    /** @brief Callbacks waiting on each in-flight URL (RequestMeshLayers path). */
    TMap<FString, TArray<FOnGlbMeshLayersLoaded>> PendingLayerCallbacks;

    /**
     * @brief Downloads Url if needed (ETag-conditional) and invokes OnReady with the local file path, or an empty string on failure.
     * @param Url     URL of the GLB file.
     * @param OnReady Called with the local path, or an empty string.
     */
    void EnsureFileOnDisk(const FString &Url, TFunction<void(const FString &FilePath)> OnReady);

    /**
     * @brief Loads a GLB file with glTFRuntime as one merged mesh, caches it and fires the waiting callbacks.
     * @param Url      URL the file was requested for (cache key).
     * @param FilePath Local path of the GLB.
     */
    void LoadMeshFromFile(const FString &Url, const FString &FilePath);

    /**
     * @brief Loads a GLB file with glTFRuntime as one mesh per node, caches them and fires the waiting callbacks.
     * @param Url      URL the file was requested for (cache key).
     * @param FilePath Local path of the GLB.
     */
    void LoadMeshLayersFromFile(const FString &Url, const FString &FilePath);

    /**
     * @brief Calls and clears every RequestMesh callback waiting on Url.
     * @param Url  URL that finished loading.
     * @param Mesh Loaded mesh, or null on failure.
     */
    void FireCallbacks(const FString &Url, UStaticMesh *Mesh);

    /**
     * @brief Calls and clears every RequestMeshLayers callback waiting on Url.
     * @param Url    URL that finished loading.
     * @param Layers Loaded layers, empty on failure.
     */
    void FireLayerCallbacks(const FString &Url, const TArray<FGlbMeshLayer> &Layers);

    /**
     * @brief Returns the cache file path for a URL.
     * @param Url URL of the GLB file.
     * @return Saved/GlbCache/ plus the URL's hash in hex plus ".glb".
     */
    FString GetCacheFilePath(const FString &Url) const;

    /** @brief Reads etags.json into ETagCache. */
    void LoadETagCache();

    /** @brief Writes ETagCache to etags.json. */
    void SaveETagCache();

    /**
     * @brief Path of the ETag index file.
     * @return Saved/GlbCache/etags.json.
     */
    static FString ETagCachePath();

    /**
     * @brief Directory that holds the cached GLB files.
     * @return Saved/GlbCache.
     */
    static FString CacheDir();
};
