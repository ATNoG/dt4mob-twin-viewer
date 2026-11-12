// Fill out your copyright notice in the Description page of Project Settings.

/**
 * @file
 * @brief GameInstance subsystem that downloads, caches and decodes sign textures and atlas JSON files.
 * @ingroup signbenchmark
 */

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SignBenchmarkTypes.h"
#include "SignAssetService.generated.h"

class UTexture2D;

/**
 * @brief On-disk cache record for one URL, persisted in etags.json. Same layout as UGlbModelService's FGlbCacheEntry.
 * @ingroup signbenchmark
 */
struct FSignCacheEntry
{
    FString ETag;      ///< ETag header of the last 200 response, sent back as If-None-Match.
    FString LocalPath; ///< Path of the downloaded file under Saved/SignBenchCache.
};

/**
 * @brief Fetches PNG textures and atlas UV JSON files over HTTP, with an ETag-validated disk cache and a session memory cache.
 *
 * Follows the same pattern as UGlbModelService but keeps its own cache directory
 * (Saved/SignBenchCache) and its own etags.json, so it cannot affect GLB loading. Besides the
 * benchmark strategies, USinalizacaoModelComponent uses it to load the AtlasMaterial atlas
 * texture and JSON for real sign entities.
 *
 * Every request first checks the session cache and answers synchronously on a hit. Otherwise it
 * sends a GET (conditional when an ETag is cached), reuses the file on disk on a 304, and decodes
 * or parses the file on the game thread. Concurrent requests for the same URL share one download.
 *
 * @note Textures are decoded into uncompressed BGRA8 transient textures with a single mip
 *       (UTexture2D::CreateTransient), not the BC-compressed streaming textures of a cooked build.
 *       This applies equally to the AtlasMaterial and PerTexture strategies, so comparing them is
 *       fair, but the absolute Texture2D_MB numbers are not representative of production.
 * @ingroup signbenchmark
 */
UCLASS()
class DT4MOB_API USignAssetService : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /**
     * @brief Creates Saved/SignBenchCache if needed and loads etags.json into ETagCache.
     * @param Collection  Subsystem collection that owns this service.
     */
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;

    /**
     * @brief Requests the texture decoded from the PNG at Url.
     *
     * @param Url       Full URL of the PNG file.
     * @param Callback  Called once with the texture, or nullptr if the download, file read or
     *                  decode failed. Called synchronously when the URL is in the session cache.
     */
    void RequestTexture(const FString &Url, TFunction<void(UTexture2D *)> Callback);

    /**
     * @brief Requests the code to UV rect table parsed from the atlas JSON at Url.
     *
     * @param Url       Full URL of the atlas JSON file.
     * @param Callback  Called once with the parsed table, or nullptr if the download, file read
     *                  or parse failed. Called synchronously when the URL is in the session cache.
     *                  The pointer refers to an entry of AtlasCache: copy the table if it must
     *                  outlive the callback.
     */
    void RequestAtlas(const FString &Url, TFunction<void(const FSignAtlasTable *)> Callback);

    /**
     * @brief Empties the session texture, atlas and ETag caches and saves the now empty etags.json.
     *
     * Benchmark helper called by ASignBenchmarkGameMode::SignBenchSetCold(), so the next request
     * for a URL already seen goes back to the network instead of returning from memory. Deleting
     * the on-disk cache directory alone leaves the in-memory maps intact. ETagCache has to go too:
     * the 304 path of EnsureFileOnDisk() returns the stored LocalPath without checking that the
     * file still exists, so a stale entry after the directory is deleted would make the load
     * fail instead of downloading the file again.
     */
    void ClearSessionCache();

private:
    /** @brief Session-only URL -> decoded texture. Filled after a successful RequestTexture() load. */
    UPROPERTY()
    TMap<FString, TObjectPtr<UTexture2D>> TextureCache;

    /** @brief Session-only URL -> parsed atlas table. Filled after a successful RequestAtlas() load. */
    UPROPERTY()
    TMap<FString, FSignAtlasTable> AtlasCache;

    /** @brief Persisted URL -> ETag and local path, shared by texture and JSON fetches (their URLs never overlap). */
    TMap<FString, FSignCacheEntry> ETagCache;

    /** @brief Callbacks waiting on each in-flight texture URL. The key exists only while a load is in flight. */
    TMap<FString, TArray<TFunction<void(UTexture2D *)>>> PendingTextureCallbacks;

    /** @brief Callbacks waiting on each in-flight atlas JSON URL. The key exists only while a load is in flight. */
    TMap<FString, TArray<TFunction<void(const FSignAtlasTable *)>>> PendingAtlasCallbacks;

    /**
     * @brief Makes sure the file at Url is in the disk cache and reports its local path.
     *
     * Sends a GET, with If-None-Match when an ETag is known. On 200 the body is written to
     * GetCacheFilePath() and, if the response has an ETag, the entry is stored and etags.json is
     * saved. On 304 the stored LocalPath is returned as is.
     *
     * @param Url        URL to fetch.
     * @param Extension  Cache file suffix without the dot ("png" or "json"). Must point to static
     *                   storage, since it is used in the completion callback.
     * @param OnReady    Called once on the game thread with the local file path, or with an empty
     *                   string on a network error, a status other than 200 or 304, a 304 without
     *                   a cache entry, or a failed file write.
     */
    void EnsureFileOnDisk(const FString &Url, const TCHAR *Extension, TFunction<void(const FString &FilePath)> OnReady);

    /**
     * @brief Reads and decodes a cached PNG, adds the texture to TextureCache on success and fires the pending callbacks.
     *
     * @param Url       URL the file was downloaded from (cache and callback key).
     * @param FilePath  Local path of the PNG.
     */
    void LoadTextureFromFile(const FString &Url, const FString &FilePath);

    /**
     * @brief Reads and parses a cached atlas JSON, adds it to AtlasCache on success and fires the pending callbacks.
     *
     * @param Url       URL the file was downloaded from (cache and callback key).
     * @param FilePath  Local path of the JSON file.
     */
    void LoadAtlasFromFile(const FString &Url, const FString &FilePath);

    /**
     * @brief Calls every callback waiting on Url, then removes them.
     *
     * @param Url      URL whose callbacks to fire.
     * @param Texture  Value passed to each callback; nullptr on failure.
     */
    void FireTextureCallbacks(const FString &Url, UTexture2D *Texture);

    /**
     * @brief Calls every callback waiting on Url, then removes them.
     *
     * @param Url    URL whose callbacks to fire.
     * @param Table  Value passed to each callback; nullptr on failure.
     */
    void FireAtlasCallbacks(const FString &Url, const FSignAtlasTable *Table);

    /**
     * @brief Decodes PNG bytes into a new transient texture.
     *
     * The texture is BGRA8 with one mip, sRGB, never streamed, trilinear filtered and clamped on
     * both axes.
     *
     * @param Bytes      Contents of a PNG file.
     * @param DebugName  Name given to the texture object.
     * @return           The texture, or nullptr if the bytes are not a decodable PNG.
     */
    static UTexture2D *DecodePngToTexture(const TArray<uint8> &Bytes, const FString &DebugName);

    /**
     * @brief Parses an atlas JSON object into a code to UV rect table.
     *
     * Expected format: `{ "C1": { "u_offset": 0.0, "v_offset": 0.0, "u_scale": 0.25, "v_scale": 0.5 }, ... }`.
     * Entries that are not objects are skipped; missing number fields default to offset 0 and
     * scale 1.
     *
     * @param JsonStr   JSON text.
     * @param OutTable  Receives one cell per entry, added to any cells already present.
     * @return          False if JsonStr is not a JSON object, true otherwise.
     */
    static bool ParseAtlasJson(const FString &JsonStr, FSignAtlasTable &OutTable);

    /**
     * @brief Returns the cache file path for Url.
     *
     * @param Url        Source URL.
     * @param Extension  File suffix without the dot.
     * @return           `Saved/SignBenchCache/<hash>.<Extension>`, where `<hash>` is the 32-bit
     *                   GetTypeHash of Url in 8 hex digits. Hash collisions are not detected.
     */
    FString GetCacheFilePath(const FString &Url, const TCHAR *Extension) const;

    /** @brief Loads etags.json into ETagCache, skipping entries without both an ETag and a path. Does nothing if the file is missing or invalid. */
    void LoadETagCache();

    /** @brief Writes ETagCache to etags.json, replacing the previous file. */
    void SaveETagCache();

    /**
     * @brief Returns the path of the persisted ETag table.
     * @return Saved/SignBenchCache/etags.json.
     */
    static FString ETagCachePath();

    /**
     * @brief Returns the disk cache directory, separate from UGlbModelService's Saved/GlbCache.
     * @return Saved/SignBenchCache under the project directory.
     */
    static FString CacheDir();
};
