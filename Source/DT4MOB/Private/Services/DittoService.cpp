// Fill out your copyright notice in the Description page of Project Settings.

/** @file DittoService.cpp
 *  @brief Implementation of UDittoService. All logic documentation is in the header.
 */
#include "Services/DittoService.h"
#include "Services/DittoSecretsAsset.h"
#include "GeotileUtils.h"
#include "UObject/SoftObjectPath.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Http.h"
#include "JsonObjectConverter.h"
#include "JsonUtilities.h"
#include "Misc/Base64.h"

// ─── Lifecycle ───────────────────────────────────────────────────────────────

void UDittoService::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    Http = &FHttpModule::Get();

    // Only non-credential defaults come from the DataAsset now — actual login happens at
    // runtime via Login(), called from the login screen (see UCredentialStoreService for the
    // persisted-credentials path that skips showing that screen).
    const UDittoSecretsAsset* Secrets = LoadObject<UDittoSecretsAsset>(
        nullptr, TEXT("/Game/Data/DA_DittoSecrets.DA_DittoSecrets"));

    if (Secrets)
    {
        DefaultHost     = Secrets->Host;
        bUseHttps       = Secrets->bUseHttps;
        bUseOAuth       = Secrets->bUseOAuth;
        OAuthClientId   = Secrets->OAuthClientId;
        WsStartMessage  = Secrets->WsStartMessage;
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("DittoService: no secrets DataAsset at /Game/Data/DA_DittoSecrets — using engine defaults, host must be supplied via the login screen"));
    }

    UE_LOG(LogTemp, Log, TEXT("DittoService initialized — awaiting login (auth=%s)"),
           bUseOAuth ? TEXT("OAuth") : TEXT("Basic"));
}

void UDittoService::Login(const FString& InHost, const FString& InUsername, const FString& InPassword,
    TFunction<void(bool)> OnComplete)
{
    Host = InHost;
    Username = InUsername;
    Password = InPassword;
    BaseUrl = (bUseHttps ? TEXT("https://") : TEXT("http://")) + Host;
    bCredentialsSet = true;

    UE_LOG(LogTemp, Log, TEXT("DittoService: logging in — user='%s' baseUrl='%s' auth=%s"),
           *Username, *BaseUrl, bUseOAuth ? TEXT("OAuth") : TEXT("Basic"));

    if (bUseOAuth)
    {
        GetOAuthToken(OnComplete);
    }
    else
    {
        // Basic auth is immediately available.
        OnAuthHeaderReady.Broadcast(GetCurrentAuthHeader());
        if (OnComplete) OnComplete(true);
    }
}

void UDittoService::Logout()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TokenRefreshTimer);
    }

    Username.Empty();
    Password.Empty();
    OAuthToken.Empty();
    RefreshToken.Empty();
    bCredentialsSet = false;
    bAuthInProgress = false;
    PendingRequests.Empty();

    UE_LOG(LogTemp, Log, TEXT("DittoService: logged out"));
    OnLoggedOut.Broadcast();
}

bool UDittoService::IsAuthenticated() const
{
    if (bUseOAuth)
        return !OAuthToken.IsEmpty();

    return bCredentialsSet;
}

void UDittoService::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TokenRefreshTimer);
    }
    Http = nullptr;
    Super::Deinitialize();
}

// ─── Authentication ───────────────────────────────────────────────────────────

void UDittoService::GetOAuthToken(TFunction<void(bool)> OnComplete)
{
    const FString Body = FString::Printf(
        TEXT("client_id=%s&grant_type=password&username=%s&password=%s"),
        *FGenericPlatformHttp::UrlEncode(OAuthClientId),
        *FGenericPlatformHttp::UrlEncode(Username),
        *FGenericPlatformHttp::UrlEncode(Password));

    SendTokenRequest(Body, OnComplete);
}

void UDittoService::RefreshOAuthToken()
{
    if (RefreshToken.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("DittoService: no refresh token available, falling back to full re-auth"));
        GetOAuthToken();
        return;
    }

    const FString Body = FString::Printf(
        TEXT("client_id=%s&grant_type=refresh_token&refresh_token=%s"),
        *FGenericPlatformHttp::UrlEncode(OAuthClientId),
        *FGenericPlatformHttp::UrlEncode(RefreshToken));

    SendTokenRequest(Body);
}

void UDittoService::SendTokenRequest(const FString& Body, TFunction<void(bool)> OnComplete)
{
    if (bAuthInProgress)
    {
        UE_LOG(LogTemp, Log, TEXT("DittoService: token request already in flight, skipping duplicate"));
        return;
    }

    bAuthInProgress = true;

    const FString Url = BaseUrl + TEXT("/auth/realms/dt4mob/protocol/openid-connect/token");
    UE_LOG(LogTemp, Log, TEXT("DittoService: requesting token from %s"), *Url);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("POST"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));
    Request->SetContentAsString(Body);

    Request->OnProcessRequestComplete().BindLambda(
        [this, OnComplete](FHttpRequestPtr, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            bAuthInProgress = false;

            if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
            {
                UE_LOG(LogTemp, Warning, TEXT("DittoService: token request failed (code %d)"),
                       Response.IsValid() ? Response->GetResponseCode() : -1);
                if (OnComplete) OnComplete(false);
                return;
            }

            TSharedPtr<FJsonObject> Json;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("DittoService: token response JSON parse failed"));
                if (OnComplete) OnComplete(false);
                return;
            }

            FString AccessToken;
            if (!Json->TryGetStringField(TEXT("access_token"), AccessToken))
            {
                UE_LOG(LogTemp, Warning, TEXT("DittoService: token response missing access_token"));
                if (OnComplete) OnComplete(false);
                return;
            }

            OAuthToken = AccessToken;
            UE_LOG(LogTemp, Log, TEXT("DittoService: access token obtained (length %d)"), OAuthToken.Len());

            FString NewRefreshToken;
            if (Json->TryGetStringField(TEXT("refresh_token"), NewRefreshToken))
            {
                RefreshToken = NewRefreshToken;
                UE_LOG(LogTemp, Log, TEXT("DittoService: refresh token obtained (length %d)"), RefreshToken.Len());
            }

            // Schedule a proactive refresh 30 s before the token expires.
            double ExpiresIn = 300.0;
            Json->TryGetNumberField(TEXT("expires_in"), ExpiresIn);
            const float RefreshDelay = FMath::Max(static_cast<float>(ExpiresIn) - 30.0f, 10.0f);

            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(
                    TokenRefreshTimer, this, &UDittoService::RefreshOAuthToken, RefreshDelay, false);
                UE_LOG(LogTemp, Log, TEXT("DittoService: token refresh scheduled in %.0f s"), RefreshDelay);
            }

            FlushPendingRequests();
            OnAuthHeaderReady.Broadcast(TEXT("Bearer ") + OAuthToken);
            if (OnComplete) OnComplete(true);
        });

    Request->ProcessRequest();
}

void UDittoService::FlushPendingRequests()
{
    if (PendingRequests.IsEmpty()) return;

    UE_LOG(LogTemp, Log, TEXT("DittoService: flushing %d queued request(s)"), PendingRequests.Num());
    TArray<TFunction<void()>> ToFlush = MoveTemp(PendingRequests);
    for (auto& Fn : ToFlush)
    {
        Fn();
    }
}

void UDittoService::SendAuthenticatedRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request)
{
    // Basic auth is ready as soon as Login() has set credentials — no token gating needed.
    if (!bUseOAuth && bCredentialsSet)
    {
        UE_LOG(LogTemp, Verbose, TEXT("DittoService: → %s %s"), *Request->GetVerb(), *Request->GetURL());
        SetCommonHeaders(Request);
        Request->ProcessRequest();
        return;
    }

    if (bUseOAuth && !OAuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Verbose, TEXT("DittoService: → %s %s"), *Request->GetVerb(), *Request->GetURL());
        SetCommonHeaders(Request);
        Request->ProcessRequest();
        return;
    }

    // Token not ready — queue and ensure auth is running.
    FString QueuedUrl = Request->GetURL();
    PendingRequests.Add([this, Request, QueuedUrl]()
    {
        UE_LOG(LogTemp, Verbose, TEXT("DittoService: flushing %s %s"),
            *Request->GetVerb(), *QueuedUrl);
        SetCommonHeaders(Request);
        Request->ProcessRequest();
    });

    // Only self-heal with a fresh token request if we actually have credentials to try (i.e.
    // Login() has been called this session) — otherwise this would fire GetOAuthToken() with
    // empty Username/Password on every stray request before the login screen completes, or
    // after an explicit Logout().
    if (bCredentialsSet && bUseOAuth && !bAuthInProgress)
    {
        UE_LOG(LogTemp, Log, TEXT("DittoService: token not ready, starting auth before queued request"));
        GetOAuthToken();
    }
}

void UDittoService::SetCommonHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request)
{
    Request->SetHeader(TEXT("Authorization"), GetCurrentAuthHeader());
}

FString UDittoService::GetCurrentAuthHeader() const
{
    if (bUseOAuth)
        return OAuthToken.IsEmpty() ? FString() : TEXT("Bearer ") + OAuthToken;

    return TEXT("Basic ") + FBase64::Encode(Username + TEXT(":") + Password);
}

// ─── API ─────────────────────────────────────────────────────────────────────

void UDittoService::GetAllThings(
    TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
    TFunction<void()> OnCompleted,
    const FString& Filter)
{
    TSharedRef<FString> Cursor = MakeShared<FString>();
    TSharedRef<TFunction<void()>> FetchPage = MakeShared<TFunction<void()>>();

    *FetchPage = [this, Cursor, OnPageReceived, OnCompleted, FetchPage, Filter]() -> void
    {
        FString Option = TEXT("size(50)");
        if (!Cursor->IsEmpty())
            Option += TEXT(",cursor(") + *Cursor + TEXT(")");

        FString Url = BaseUrl + TEXT("/api/2/search/things?option=") + Option;
        if (!Filter.IsEmpty())
            Url += TEXT("&filter=") + Filter;

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
        Request->SetURL(Url);

        Request->SetVerb(TEXT("GET"));

        Request->OnProcessRequestComplete().BindLambda(
            [Cursor, OnPageReceived, OnCompleted, FetchPage](FHttpRequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful)
            {
                if (!bWasSuccessful || !ResponsePtr.IsValid())
                {
                    UE_LOG(LogTemp, Error, TEXT("DittoService::GetAllThings: HTTP request failed"));
                    if (OnCompleted) OnCompleted();
                    return;
                }

                const int32 Code = ResponsePtr->GetResponseCode();
                if (Code != 200)
                {
                    UE_LOG(LogTemp, Error, TEXT("DittoService::GetAllThings: HTTP %d — %s"),
                           Code, *ResponsePtr->GetContentAsString().Left(512));
                    if (OnCompleted) OnCompleted();
                    return;
                }

                TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponsePtr->GetContentAsString());
                if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
                {
                    UE_LOG(LogTemp, Error, TEXT("DittoService::GetAllThings: JSON parse failed"));
                    if (OnCompleted) OnCompleted();
                    return;
                }

                TArray<TSharedPtr<FJsonObject>> PageItems;
                const TArray<TSharedPtr<FJsonValue>>* ItemsArray;
                if (JsonObject->TryGetArrayField(TEXT("items"), ItemsArray))
                {
                    PageItems.Reserve(ItemsArray->Num());
                    for (const TSharedPtr<FJsonValue>& Item : *ItemsArray)
                    {
                        TSharedPtr<FJsonObject> Obj = Item->AsObject();
                        if (Obj.IsValid()) PageItems.Add(Obj);
                    }
                }

                UE_LOG(LogTemp, Verbose, TEXT("DittoService: ← GetAllThings page: %d items, cursor=%s"),
                       PageItems.Num(), JsonObject->HasField(TEXT("cursor")) ? TEXT("yes") : TEXT("no"));

                if (OnPageReceived) OnPageReceived(PageItems);

                FString NewCursor;
                if (JsonObject->TryGetStringField(TEXT("cursor"), NewCursor))
                {
                    *Cursor = NewCursor;
                    (*FetchPage)();
                }
                else
                {
                    if (OnCompleted) OnCompleted();
                }
            });

        SendAuthenticatedRequest(Request);
    };

    (*FetchPage)();
}

void UDittoService::GetThingsByGeotile(
    double Lat,
    double Lng,
    int32 TileZoom,
    TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
    TFunction<void()> OnCompleted)
{
    int64 Lower, Upper;
    GetTileBounds(Lat, Lng, TileZoom, Lower, Upper);
    GetThingsByGeotileBounds(Lower, Upper, MoveTemp(OnPageReceived), MoveTemp(OnCompleted));
}

void UDittoService::GetThingsByGeotileBounds(
    int64 Lower,
    int64 Upper,
    TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
    TFunction<void()> OnCompleted)
{
    // Upper is the exclusive start of the next tile's range (see GetTileBoundsFromKey) — use
    // lt, not le, or an entity sitting exactly on a tile boundary gets returned (and spawned)
    // by both adjacent tile fetches.
    //
    // Also exclude already-expired things: a finished TRACI run leaves its whole fleet in
    // Ditto's DB (expired, not deleted), and without this a tile fetch drags the entire
    // graveyard back — hundreds of dead vehicles spawned at once. Things with no expiry_ts
    // (static infra) are kept via not(exists(...)). 30s slack keeps this loose; the
    // authoritative reject is client-side in UDT4MOBEntityFactory::SpawnTempUIActor().
    const FString NowMinusSlackIso = (FDateTime::UtcNow() - FTimespan::FromSeconds(30.0)).ToIso8601();
    const FString Filter = FString::Printf(
        TEXT("and(ge(attributes/geotile,%lld),lt(attributes/geotile,%lld),")
        TEXT("or(not(exists(attributes/expiry_ts)),gt(attributes/expiry_ts,\"%s\")))"),
        Lower, Upper, *NowMinusSlackIso);

    TSharedRef<FString> Cursor = MakeShared<FString>();
    TSharedRef<TFunction<void()>> FetchPage = MakeShared<TFunction<void()>>();

    *FetchPage = [this, Filter, Cursor, OnPageReceived, OnCompleted, FetchPage]() -> void
    {
        const FString BaseRequestURL = BaseUrl
            + TEXT("/api/2/search/things?filter=") + FGenericPlatformHttp::UrlEncode(Filter)
            + TEXT("&option=size(200)");

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();

        if (!Cursor->IsEmpty())
            Request->SetURL(BaseRequestURL + TEXT(",cursor(") + *Cursor + TEXT(")"));
        else
            Request->SetURL(BaseRequestURL);

        Request->SetVerb(TEXT("GET"));

        Request->OnProcessRequestComplete().BindLambda(
            [Cursor, OnPageReceived, OnCompleted, FetchPage](FHttpRequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful)
            {
                if (!bWasSuccessful || !ResponsePtr.IsValid())
                {
                    UE_LOG(LogTemp, Error, TEXT("DittoService::GetThingsByGeotileBounds: HTTP request failed"));
                    if (OnCompleted) OnCompleted();
                    return;
                }

                TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponsePtr->GetContentAsString());
                if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
                {
                    UE_LOG(LogTemp, Error, TEXT("DittoService::GetThingsByGeotileBounds: JSON parse failed"));
                    if (OnCompleted) OnCompleted();
                    return;
                }

                TArray<TSharedPtr<FJsonObject>> PageItems;
                const TArray<TSharedPtr<FJsonValue>>* ItemsArray;
                if (JsonObject->TryGetArrayField(TEXT("items"), ItemsArray))
                {
                    PageItems.Reserve(ItemsArray->Num());
                    for (const TSharedPtr<FJsonValue>& Item : *ItemsArray)
                    {
                        TSharedPtr<FJsonObject> Obj = Item->AsObject();
                        if (Obj.IsValid()) PageItems.Add(Obj);
                    }
                }

                if (OnPageReceived) OnPageReceived(PageItems);

                FString NewCursor;
                if (JsonObject->TryGetStringField(TEXT("cursor"), NewCursor))
                {
                    *Cursor = NewCursor;
                    (*FetchPage)();
                }
                else
                {
                    if (OnCompleted) OnCompleted();
                }
            });

        SendAuthenticatedRequest(Request);
    };

    (*FetchPage)();
}

void UDittoService::GetThingById(
    const FString& ThingId,
    TFunction<void(TSharedPtr<FJsonObject>)> OnComplete)
{
    const FString Url = BaseUrl + TEXT("/api/2/things/") + FGenericPlatformHttp::UrlEncode(ThingId);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("GET"));

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete, ThingId](FHttpRequestPtr, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            if (!bWasSuccessful || !Response.IsValid() || Response->GetResponseCode() != 200)
            {
                UE_LOG(LogTemp, Warning, TEXT("DittoService::GetThingById [%s] failed (code %d)"),
                       *ThingId, Response.IsValid() ? Response->GetResponseCode() : -1);
                if (OnComplete) OnComplete(nullptr);
                return;
            }

            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("DittoService::GetThingById [%s] JSON parse failed"), *ThingId);
                if (OnComplete) OnComplete(nullptr);
                return;
            }

            if (OnComplete) OnComplete(JsonObject);
        });

    SendAuthenticatedRequest(Request);
}

void UDittoService::PutThing(
    const FString& ThingId,
    TSharedPtr<FJsonObject> Body,
    TFunction<void(bool)> OnComplete)
{
    FString BodyString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
    if (!FJsonSerializer::Serialize(Body.ToSharedRef(), Writer))
    {
        if (OnComplete) OnComplete(false);
        return;
    }

    const FString Url = BaseUrl + TEXT("/api/2/things/") + FGenericPlatformHttp::UrlEncode(ThingId);

    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb(TEXT("PUT"));
    Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
    Request->SetContentAsString(BodyString);

    UE_LOG(LogTemp, Log, TEXT("DittoService::PutThing [%s]"), *ThingId);

    Request->OnProcessRequestComplete().BindLambda(
        [OnComplete, ThingId](FHttpRequestPtr, FHttpResponsePtr Response, bool bWasSuccessful)
        {
            const bool bSuccess = bWasSuccessful && Response.IsValid()
                && Response->GetResponseCode() >= 200 && Response->GetResponseCode() < 300;
            UE_LOG(LogTemp, Log, TEXT("DittoService::PutThing [%s] → %d"),
                   *ThingId, Response.IsValid() ? Response->GetResponseCode() : -1);
            if (OnComplete) OnComplete(bSuccess);
        });

    SendAuthenticatedRequest(Request);
}

// ─── Geo math ────────────────────────────────────────────────────────────────

void UDittoService::GetTileXY(double Lat, double Lng, int32 Zoom, int64& OutX, int64& OutY)
{
    FGeotileUtils::LatLonToTileXY(Lat, Lng, Zoom, OutX, OutY);
}

int64 UDittoService::GetQuadkeyFromXY(int64 X, int64 Y, int32 Zoom)
{
    return FGeotileUtils::TileXYToQuadkey(X, Y, Zoom);
}

int64 UDittoService::GetQuadkey(double Lat, double Lng, int32 Zoom)
{
    return FGeotileUtils::LatLonToGeotile(Lat, Lng, Zoom);
}

void UDittoService::GetTileBoundsFromKey(int64 QuadKey, int32 TileZoom, int64& OutLower, int64& OutUpper, int32 MaxZoom)
{
    if (TileZoom <= MaxZoom)
    {
        const int32 ShiftBits = 2 * (MaxZoom - TileZoom);
        OutLower = QuadKey << ShiftBits;
        OutUpper = (QuadKey + 1) << ShiftBits;
    }
    else
    {
        // Requested tile is finer than the zoom level Ditto stores geotiles at —
        // collapse down to the single MaxZoom-precision tile that contains it,
        // since no query can narrow past the data's own precision.
        const int32 ShiftBits = 2 * (TileZoom - MaxZoom);
        const int64 CollapsedKey = QuadKey >> ShiftBits;
        OutLower = CollapsedKey;
        OutUpper = CollapsedKey + 1;
    }
}

void UDittoService::GetTileBounds(double Lat, double Lng, int32 TileZoom, int64& OutLower, int64& OutUpper, int32 MaxZoom)
{
    FGeotileUtils::GeotileBounds(Lat, Lng, TileZoom, OutLower, OutUpper, MaxZoom);
}

int32 UDittoService::AltitudeToZoomLevel(double AltitudeMeters)
{
    if (AltitudeMeters <= 1.0) return MaxTileZoom;
    const double Z = FMath::Log2(10019000.0 / AltitudeMeters);
    return FMath::Clamp(FMath::RoundToInt(Z), 0, MaxTileZoom);
}
