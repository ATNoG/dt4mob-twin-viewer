#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "glTFRuntimeFunctionLibrary.h"
#include "glTFRuntimeAsset.h"
#include "GlbModelService.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGlbMeshLoaded, UStaticMesh *, Mesh);

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
 * First request: GET → save body to disk → store ETag in etags.json → load mesh.
 * Subsequent requests (same session): served instantly from the in-memory MeshCache.
 * Subsequent requests (new session): GET with If-None-Match → 304 → load from disk.
 * Content changed: server returns 200 with new ETag → overwrite file → reload mesh.
 *
 * Multiple actors may call RequestMesh() for the same URL concurrently; only one
 * HTTP request is made and all callbacks fire together on completion.
 */
UCLASS()
class DT4MOB_API UGlbModelService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;

    /**
     * @brief Request a UStaticMesh for Url.
     * Fires Callback immediately if the mesh is in the session cache,
     * otherwise starts (or joins an in-flight) HTTP request.
     */
    void RequestMesh(const FString &Url, const FOnGlbMeshLoaded &Callback);

private:
    /** @brief Session-only URL → mesh map. Populated after a successful load. */
    UPROPERTY()
    TMap<FString, UStaticMesh *> MeshCache;

    /** @brief Persisted URL → {ETag, LocalPath}. Loaded from / saved to disk. */
    TMap<FString, FGlbCacheEntry> ETagCache;

    /** @brief Callbacks waiting on each in-flight URL. */
    TMap<FString, TArray<FOnGlbMeshLoaded>> PendingCallbacks;

    void StartHttpRequest(const FString &Url);
    void OnHttpResponse(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess, FString Url);
    void LoadMeshFromFile(const FString &Url, const FString &FilePath);
    void FireCallbacks(const FString &Url, UStaticMesh *Mesh);

    FString GetCacheFilePath(const FString &Url) const;
    void LoadETagCache();
    void SaveETagCache();

    static FString ETagCachePath();
    static FString CacheDir();
};
