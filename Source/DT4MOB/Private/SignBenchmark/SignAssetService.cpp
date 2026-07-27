#include "SignBenchmark/SignAssetService.h"
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
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Modules/ModuleManager.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"

// ============================================================
//  Helpers
// ============================================================

FString USignAssetService::CacheDir()
{
    return FPaths::ProjectSavedDir() / TEXT("SignBenchCache");
}

FString USignAssetService::ETagCachePath()
{
    return CacheDir() / TEXT("etags.json");
}

FString USignAssetService::GetCacheFilePath(const FString &Url, const TCHAR *Extension) const
{
    return CacheDir() / FString::Printf(TEXT("%08X.%s"), GetTypeHash(Url), Extension);
}

// ============================================================
//  Lifecycle
// ============================================================

void USignAssetService::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    IPlatformFile &PF = FPlatformFileManager::Get().GetPlatformFile();
    if (!PF.DirectoryExists(*CacheDir()))
        PF.CreateDirectoryTree(*CacheDir());

    LoadETagCache();
    UE_LOG(LogTemp, Log, TEXT("SignAssetService: initialized, %d cached entries"), ETagCache.Num());
}

// ============================================================
//  ETag persistence
// ============================================================

void USignAssetService::LoadETagCache()
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

        FSignCacheEntry Entry;
        (*EntryObj)->TryGetStringField(TEXT("etag"), Entry.ETag);
        (*EntryObj)->TryGetStringField(TEXT("path"), Entry.LocalPath);

        if (!Entry.ETag.IsEmpty() && !Entry.LocalPath.IsEmpty())
            ETagCache.Add(Pair.Key, Entry);
    }
}

void USignAssetService::SaveETagCache()
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

void USignAssetService::RequestTexture(const FString &Url, TFunction<void(UTexture2D *)> Callback)
{
    if (TObjectPtr<UTexture2D> *Cached = TextureCache.Find(Url))
    {
        Callback(*Cached);
        return;
    }

    const bool bAlreadyLoading = PendingTextureCallbacks.Contains(Url);
    PendingTextureCallbacks.FindOrAdd(Url).Add(MoveTemp(Callback));
    if (bAlreadyLoading)
        return;

    EnsureFileOnDisk(Url, TEXT("png"), [this, Url](const FString &FilePath)
    {
        if (FilePath.IsEmpty())
        {
            FireTextureCallbacks(Url, nullptr);
            return;
        }
        LoadTextureFromFile(Url, FilePath);
    });
}

void USignAssetService::RequestAtlas(const FString &Url, TFunction<void(const FSignAtlasTable *)> Callback)
{
    if (FSignAtlasTable *Cached = AtlasCache.Find(Url))
    {
        Callback(Cached);
        return;
    }

    const bool bAlreadyLoading = PendingAtlasCallbacks.Contains(Url);
    PendingAtlasCallbacks.FindOrAdd(Url).Add(MoveTemp(Callback));
    if (bAlreadyLoading)
        return;

    EnsureFileOnDisk(Url, TEXT("json"), [this, Url](const FString &FilePath)
    {
        if (FilePath.IsEmpty())
        {
            FireAtlasCallbacks(Url, nullptr);
            return;
        }
        LoadAtlasFromFile(Url, FilePath);
    });
}

// ============================================================
//  HTTP request (shared by RequestTexture and RequestAtlas)
// ============================================================

void USignAssetService::EnsureFileOnDisk(const FString &Url, const TCHAR *Extension, TFunction<void(const FString &FilePath)> OnReady)
{
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Req = FHttpModule::Get().CreateRequest();
    Req->SetURL(Url);
    Req->SetVerb(TEXT("GET"));

    if (const FSignCacheEntry *Entry = ETagCache.Find(Url))
    {
        if (!Entry->ETag.IsEmpty())
            Req->SetHeader(TEXT("If-None-Match"), Entry->ETag);
    }

    Req->OnProcessRequestComplete().BindLambda(
        [this, Url, Extension, OnReady](FHttpRequestPtr /*Request*/, FHttpResponsePtr Response, bool bSuccess)
        {
            if (!bSuccess || !Response.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("SignAssetService: request failed for %s"), *Url);
                OnReady(FString());
                return;
            }

            const int32 Code = Response->GetResponseCode();

            if (Code == 304)
            {
                if (const FSignCacheEntry *Entry = ETagCache.Find(Url))
                {
                    UE_LOG(LogTemp, Log, TEXT("SignAssetService: 304 for %s, loading from %s"), *Url, *Entry->LocalPath);
                    OnReady(Entry->LocalPath);
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("SignAssetService: 304 but no local file for %s"), *Url);
                    OnReady(FString());
                }
                return;
            }

            if (Code != 200)
            {
                UE_LOG(LogTemp, Warning, TEXT("SignAssetService: HTTP %d for %s"), Code, *Url);
                OnReady(FString());
                return;
            }

            const FString FilePath = GetCacheFilePath(Url, Extension);
            if (!FFileHelper::SaveArrayToFile(Response->GetContent(), *FilePath))
            {
                UE_LOG(LogTemp, Warning, TEXT("SignAssetService: failed to write cache file %s"), *FilePath);
                OnReady(FString());
                return;
            }

            const FString ETag = Response->GetHeader(TEXT("ETag"));
            if (!ETag.IsEmpty())
            {
                ETagCache.Add(Url, {ETag, FilePath});
                SaveETagCache();
                UE_LOG(LogTemp, Log, TEXT("SignAssetService: stored ETag '%s' for %s"), *ETag, *Url);
            }

            OnReady(FilePath);
        });

    Req->ProcessRequest();
    UE_LOG(LogTemp, Log, TEXT("SignAssetService: GET %s"), *Url);
}

// ============================================================
//  Texture decode + callback dispatch
// ============================================================

UTexture2D *USignAssetService::DecodePngToTexture(const TArray<uint8> &Bytes, const FString &DebugName)
{
    IImageWrapperModule &ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
    TSharedPtr<IImageWrapper> Wrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
    if (!Wrapper.IsValid() || !Wrapper->SetCompressed(Bytes.GetData(), Bytes.Num()))
        return nullptr;

    TArray64<uint8> Raw;
    if (!Wrapper->GetRaw(ERGBFormat::BGRA, 8, Raw))
        return nullptr;

    UTexture2D *Texture = UTexture2D::CreateTransient(Wrapper->GetWidth(), Wrapper->GetHeight(), PF_B8G8R8A8, *DebugName);
    if (!Texture)
        return nullptr;

    Texture->SRGB = true;
    Texture->NeverStream = true;
    Texture->Filter = TF_Trilinear;
    Texture->AddressX = TA_Clamp; // prevents atlas neighbour cells bleeding in at edges/mips
    Texture->AddressY = TA_Clamp;

    void *Dst = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(Dst, Raw.GetData(), Raw.Num());
    Texture->GetPlatformData()->Mips[0].BulkData.Unlock();
    Texture->UpdateResource();

    return Texture;
}

void USignAssetService::LoadTextureFromFile(const FString &Url, const FString &FilePath)
{
    TArray<uint8> Bytes;
    UTexture2D *Texture = nullptr;

    if (FFileHelper::LoadFileToArray(Bytes, *FilePath))
    {
        Texture = DecodePngToTexture(Bytes, FPaths::GetBaseFilename(FilePath));
        if (Texture)
            TextureCache.Add(Url, Texture);
    }

    if (!Texture)
        UE_LOG(LogTemp, Warning, TEXT("SignAssetService: texture decode failed from %s"), *FilePath);

    FireTextureCallbacks(Url, Texture);
}

void USignAssetService::FireTextureCallbacks(const FString &Url, UTexture2D *Texture)
{
    TArray<TFunction<void(UTexture2D *)>> *Callbacks = PendingTextureCallbacks.Find(Url);
    if (!Callbacks)
        return;

    for (const TFunction<void(UTexture2D *)> &Cb : *Callbacks)
        Cb(Texture);

    PendingTextureCallbacks.Remove(Url);
}

// ============================================================
//  Atlas JSON parse + callback dispatch
// ============================================================

bool USignAssetService::ParseAtlasJson(const FString &JsonStr, FSignAtlasTable &OutTable)
{
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
        return false;

    for (const auto &Pair : Root->Values)
    {
        const TSharedPtr<FJsonObject> *CellObj = nullptr;
        if (!Root->TryGetObjectField(Pair.Key, CellObj) || !CellObj)
            continue;

        FSignAtlasUV UV;
        double UOffset = 0.0, VOffset = 0.0, UScale = 1.0, VScale = 1.0;
        (*CellObj)->TryGetNumberField(TEXT("u_offset"), UOffset);
        (*CellObj)->TryGetNumberField(TEXT("v_offset"), VOffset);
        (*CellObj)->TryGetNumberField(TEXT("u_scale"), UScale);
        (*CellObj)->TryGetNumberField(TEXT("v_scale"), VScale);

        UV.UOffset = static_cast<float>(UOffset);
        UV.VOffset = static_cast<float>(VOffset);
        UV.UScale = static_cast<float>(UScale);
        UV.VScale = static_cast<float>(VScale);

        OutTable.Cells.Add(Pair.Key, UV);
    }

    return true;
}

void USignAssetService::LoadAtlasFromFile(const FString &Url, const FString &FilePath)
{
    FString JsonStr;
    FSignAtlasTable Table;
    bool bOk = false;

    if (FFileHelper::LoadFileToString(JsonStr, *FilePath))
        bOk = ParseAtlasJson(JsonStr, Table);

    if (!bOk)
    {
        UE_LOG(LogTemp, Warning, TEXT("SignAssetService: atlas JSON parse failed from %s"), *FilePath);
        FireAtlasCallbacks(Url, nullptr);
        return;
    }

    FSignAtlasTable &Cached = AtlasCache.Add(Url, MoveTemp(Table));
    FireAtlasCallbacks(Url, &Cached);
}

void USignAssetService::FireAtlasCallbacks(const FString &Url, const FSignAtlasTable *Table)
{
    TArray<TFunction<void(const FSignAtlasTable *)>> *Callbacks = PendingAtlasCallbacks.Find(Url);
    if (!Callbacks)
        return;

    for (const TFunction<void(const FSignAtlasTable *)> &Cb : *Callbacks)
        Cb(Table);

    PendingAtlasCallbacks.Remove(Url);
}
