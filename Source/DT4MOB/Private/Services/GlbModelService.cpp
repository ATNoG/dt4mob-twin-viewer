#include "Services/GlbModelService.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "glTFRuntimeParser.h"

// ============================================================
//  Helpers
// ============================================================

FString UGlbModelService::CacheDir()
{
    return FPaths::ProjectSavedDir() / TEXT("GlbCache");
}

FString UGlbModelService::ETagCachePath()
{
    return CacheDir() / TEXT("etags.json");
}

FString UGlbModelService::GetCacheFilePath(const FString &Url) const
{
    return CacheDir() / FString::Printf(TEXT("%08X.glb"), GetTypeHash(Url));
}

// ============================================================
//  Lifecycle
// ============================================================

void UGlbModelService::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    IPlatformFile &PF = FPlatformFileManager::Get().GetPlatformFile();
    if (!PF.DirectoryExists(*CacheDir()))
        PF.CreateDirectoryTree(*CacheDir());

    LoadETagCache();
    UE_LOG(LogTemp, Log, TEXT("GlbModelService: initialized, %d cached entries"), ETagCache.Num());
}

// ============================================================
//  ETag persistence
// ============================================================

void UGlbModelService::LoadETagCache()
{
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *ETagCachePath()))
        return;

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
        return;

    for (const auto &Pair : Root->Values)
    {
        const TSharedPtr<FJsonObject> *EntryObj = nullptr;
        if (!Root->TryGetObjectField(Pair.Key, EntryObj) || !EntryObj)
            continue;

        FGlbCacheEntry Entry;
        (*EntryObj)->TryGetStringField(TEXT("etag"), Entry.ETag);
        (*EntryObj)->TryGetStringField(TEXT("path"), Entry.LocalPath);

        if (!Entry.ETag.IsEmpty() && !Entry.LocalPath.IsEmpty())
            ETagCache.Add(Pair.Key, Entry);
    }
}

void UGlbModelService::SaveETagCache()
{
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();

    for (const auto &Pair : ETagCache)
    {
        TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
        Entry->SetStringField(TEXT("etag"), Pair.Value.ETag);
        Entry->SetStringField(TEXT("path"), Pair.Value.LocalPath);
        Root->SetObjectField(Pair.Key, Entry);
    }

    FString JsonStr;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
    FJsonSerializer::Serialize(Root.ToSharedRef(), Writer);
    FFileHelper::SaveStringToFile(JsonStr, *ETagCachePath());
}

// ============================================================
//  Public API
// ============================================================

void UGlbModelService::RequestMesh(const FString &Url, const FOnGlbMeshLoaded &Callback)
{
    // Session cache hit — no I/O at all
    if (UStaticMesh **Cached = MeshCache.Find(Url))
    {
        Callback.ExecuteIfBound(*Cached);
        return;
    }

    // Queue callback; if a request for this URL is already in flight, just wait
    bool bAlreadyLoading = PendingCallbacks.Contains(Url);
    PendingCallbacks.FindOrAdd(Url).Add(Callback);
    if (bAlreadyLoading)
        return;

    StartHttpRequest(Url);
}

// ============================================================
//  HTTP request
// ============================================================

void UGlbModelService::StartHttpRequest(const FString &Url)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
    Req->SetURL(Url);
    Req->SetVerb(TEXT("GET"));

    // Conditional request — skip the download if server content hasn't changed
    if (const FGlbCacheEntry *Entry = ETagCache.Find(Url))
    {
        if (!Entry->ETag.IsEmpty())
            Req->SetHeader(TEXT("If-None-Match"), Entry->ETag);
    }

    Req->OnProcessRequestComplete().BindLambda(
        [this, Url](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
        {
            OnHttpResponse(Request, Response, bSuccess, Url);
        });

    Req->ProcessRequest();
    UE_LOG(LogTemp, Log, TEXT("GlbModelService: GET %s"), *Url);
}

void UGlbModelService::OnHttpResponse(FHttpRequestPtr /*Request*/, FHttpResponsePtr Response,
                                       bool bSuccess, FString Url)
{
    if (!bSuccess || !Response.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("GlbModelService: request failed for %s"), *Url);
        FireCallbacks(Url, nullptr);
        return;
    }

    const int32 Code = Response->GetResponseCode();

    // ---- 304 Not Modified — file on disk is still current ----
    if (Code == 304)
    {
        if (const FGlbCacheEntry *Entry = ETagCache.Find(Url))
        {
            UE_LOG(LogTemp, Log, TEXT("GlbModelService: 304 for %s, loading from %s"),
                   *Url, *Entry->LocalPath);
            LoadMeshFromFile(Url, Entry->LocalPath);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("GlbModelService: 304 but no local file for %s"), *Url);
            FireCallbacks(Url, nullptr);
        }
        return;
    }

    if (Code != 200)
    {
        UE_LOG(LogTemp, Warning, TEXT("GlbModelService: HTTP %d for %s"), Code, *Url);
        FireCallbacks(Url, nullptr);
        return;
    }

    // ---- 200 OK — save body, update ETag ----
    FString FilePath = GetCacheFilePath(Url);
    if (!FFileHelper::SaveArrayToFile(Response->GetContent(), *FilePath))
    {
        UE_LOG(LogTemp, Warning, TEXT("GlbModelService: failed to write cache file %s"), *FilePath);
        FireCallbacks(Url, nullptr);
        return;
    }

    FString ETag = Response->GetHeader(TEXT("ETag"));
    if (!ETag.IsEmpty())
    {
        ETagCache.Add(Url, {ETag, FilePath});
        SaveETagCache();
        UE_LOG(LogTemp, Log, TEXT("GlbModelService: stored ETag '%s' for %s"), *ETag, *Url);
    }

    LoadMeshFromFile(Url, FilePath);
}

// ============================================================
//  Mesh loading + callback dispatch
// ============================================================

void UGlbModelService::LoadMeshFromFile(const FString &Url, const FString &FilePath)
{
    FglTFRuntimeConfig Config;
    UglTFRuntimeAsset *Asset = UglTFRuntimeFunctionLibrary::glTFLoadAssetFromFilename(
        FilePath, /*bPathRelativeToContent=*/false, Config);

    UStaticMesh *Mesh = nullptr;
    if (Asset)
    {
        FglTFRuntimeStaticMeshConfig MeshConfig;
        Mesh = Asset->LoadStaticMeshRecursive(TEXT(""), TArray<FString>(), MeshConfig);
        if (Mesh)
            MeshCache.Add(Url, Mesh);
    }

    if (!Mesh)
        UE_LOG(LogTemp, Warning, TEXT("GlbModelService: mesh load failed from %s"), *FilePath);

    FireCallbacks(Url, Mesh);
}

void UGlbModelService::FireCallbacks(const FString &Url, UStaticMesh *Mesh)
{
    TArray<FOnGlbMeshLoaded> *Callbacks = PendingCallbacks.Find(Url);
    if (!Callbacks)
        return;

    for (const FOnGlbMeshLoaded &Cb : *Callbacks)
        Cb.ExecuteIfBound(Mesh);

    PendingCallbacks.Remove(Url);
}
