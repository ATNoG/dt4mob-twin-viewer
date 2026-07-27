// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SignBenchmarkTypes.h"
#include "SignAssetService.generated.h"

class UTexture2D;

/** @brief Persistent per-URL entry written to disk, same shape as GlbModelService's own cache entry. */
struct FSignCacheEntry
{
    FString ETag;
    FString LocalPath;
};

/**
 * @brief GameInstance subsystem that fetches PNG textures and atlas-UV JSON
 * sidecars over HTTP with ETag-based disk caching, mirroring UGlbModelService's
 * pattern. Deliberately a separate cache/service from UGlbModelService (own
 * cache dir, own etags.json) rather than a shared refactor, so this throwaway
 * benchmark code can never affect production GLB loading.
 *
 * Note for the thesis methodology section: textures are decoded into
 * uncompressed, single-mip UTexture2D::CreateTransient() textures — not
 * representative of a cooked build's BC-compressed streaming textures. This
 * applies equally to both the AtlasMaterial and PerTexture strategies, so the
 * comparison between them stays apples-to-apples even though the absolute
 * memory numbers aren't production-representative.
 */
UCLASS()
class DT4MOB_API USignAssetService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;

    /** @brief Request a UTexture2D decoded from the PNG at Url. Fires Callback immediately if cached. */
    void RequestTexture(const FString &Url, TFunction<void(UTexture2D *)> Callback);

    /** @brief Request the parsed atlas UV table (code -> UV rect) from the JSON at Url. Fires Callback immediately if cached. */
    void RequestAtlas(const FString &Url, TFunction<void(const FSignAtlasTable *)> Callback);

private:
    /** @brief Session-only URL -> decoded texture. Populated after a successful RequestTexture load. */
    UPROPERTY()
    TMap<FString, TObjectPtr<UTexture2D>> TextureCache;

    /** @brief Session-only URL -> parsed atlas table. Populated after a successful RequestAtlas load. */
    UPROPERTY()
    TMap<FString, FSignAtlasTable> AtlasCache;

    /** @brief Persisted URL -> {ETag, LocalPath}, shared by both texture and JSON fetches (URLs are unique across both). */
    TMap<FString, FSignCacheEntry> ETagCache;

    /** @brief Callbacks waiting on each in-flight URL (RequestTexture path). */
    TMap<FString, TArray<TFunction<void(UTexture2D *)>>> PendingTextureCallbacks;

    /** @brief Callbacks waiting on each in-flight URL (RequestAtlas path). */
    TMap<FString, TArray<TFunction<void(const FSignAtlasTable *)>>> PendingAtlasCallbacks;

    /** @brief Downloads Url if needed (ETag-conditional) and invokes OnReady with the local file path, or an empty string on failure. Extension is only used for the cache filename's suffix. */
    void EnsureFileOnDisk(const FString &Url, const TCHAR *Extension, TFunction<void(const FString &FilePath)> OnReady);

    void LoadTextureFromFile(const FString &Url, const FString &FilePath);
    void LoadAtlasFromFile(const FString &Url, const FString &FilePath);

    void FireTextureCallbacks(const FString &Url, UTexture2D *Texture);
    void FireAtlasCallbacks(const FString &Url, const FSignAtlasTable *Table);

    static UTexture2D *DecodePngToTexture(const TArray<uint8> &Bytes, const FString &DebugName);
    static bool ParseAtlasJson(const FString &JsonStr, FSignAtlasTable &OutTable);

    FString GetCacheFilePath(const FString &Url, const TCHAR *Extension) const;
    void LoadETagCache();
    void SaveETagCache();

    static FString ETagCachePath();
    static FString CacheDir();
};
