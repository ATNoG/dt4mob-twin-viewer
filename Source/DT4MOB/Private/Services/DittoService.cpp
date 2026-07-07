// Fill out your copyright notice in the Description page of Project Settings.

/** @file DittoService.cpp
 *  @brief Implementation of UDittoService. All logic documentation is in the header.
 */
#include "Services/DittoService.h"
#include "GeotileUtils.h"
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

    const FString SecretsFile = FPaths::ProjectConfigDir() / TEXT("Secrets.ini");
    GConfig->LoadFile(SecretsFile);

    FString Host;
    bool bUseHttps = true;
    GConfig->GetString(TEXT("Ditto"), TEXT("Username"), Username,  SecretsFile);
    GConfig->GetString(TEXT("Ditto"), TEXT("Password"), Password,  SecretsFile);
    GConfig->GetString(TEXT("Ditto"), TEXT("Host"),     Host,      SecretsFile);
    GConfig->GetBool  (TEXT("Ditto"), TEXT("UseHttps"), bUseHttps, SecretsFile);
    GConfig->GetBool  (TEXT("Ditto"), TEXT("UseOAuth"), bUseOAuth, SecretsFile);

    BaseUrl = (bUseHttps ? TEXT("https://") : TEXT("http://")) + Host;

    if (Username.IsEmpty() || Password.IsEmpty() || Host.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("DittoService: one or more values missing from Config/Secrets.ini"));
    }

    UE_LOG(LogTemp, Log, TEXT("DittoService initialized — user='%s' baseUrl='%s' auth=%s"),
           *Username, *BaseUrl, bUseOAuth ? TEXT("OAuth") : TEXT("Basic"));

    if (bUseOAuth)
    {
        GetOAuthToken();
    }
    else
    {
        // Basic auth is immediately available — notify subscribers now.
        OnAuthHeaderReady.Broadcast(GetCurrentAuthHeader());
    }
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

void UDittoService::GetOAuthToken()
{
    const FString Body = FString::Printf(
        TEXT("client_id=%s&grant_type=password&username=%s&password=%s"),
        *FGenericPlatformHttp::UrlEncode(TEXT("ditto")),
        *FGenericPlatformHttp::UrlEncode(Username),
        *FGenericPlatformHttp::UrlEncode(Password));

    SendTokenRequest(Body);
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
        *FGenericPlatformHttp::UrlEncode(TEXT("ditto")),
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
    // Basic auth is always ready — no token gating needed.
    if (!bUseOAuth)
    {
        UE_LOG(LogTemp, Log, TEXT("DittoService: → %s %s"), *Request->GetVerb(), *Request->GetURL());
        SetCommonHeaders(Request);
        Request->ProcessRequest();
        return;
    }

    if (!OAuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Log, TEXT("DittoService: → %s %s"), *Request->GetVerb(), *Request->GetURL());
        SetCommonHeaders(Request);
        Request->ProcessRequest();
        return;
    }

    // Token not ready — queue and ensure auth is running.
    FString QueuedUrl = Request->GetURL();
    PendingRequests.Add([this, Request, QueuedUrl]()
    {
        UE_LOG(LogTemp, Log, TEXT("DittoService: flushing %s %s"),
            *Request->GetVerb(), *QueuedUrl);
        SetCommonHeaders(Request);
        Request->ProcessRequest();
    });

    if (!bAuthInProgress)
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
    TFunction<void()> OnCompleted)
{
    TSharedRef<FString> Cursor = MakeShared<FString>();
    TSharedRef<TFunction<void()>> FetchPage = MakeShared<TFunction<void()>>();

    *FetchPage = [this, Cursor, OnPageReceived, OnCompleted, FetchPage]() -> void
    {
        const FString BaseRequestURL = BaseUrl + TEXT("/api/2/search/things?option=size(50)");

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

                UE_LOG(LogTemp, Log, TEXT("DittoService: ← GetAllThings page: %d items, cursor=%s"),
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
    const FString Filter = FString::Printf(
        TEXT("and(ge(attributes/geotile,%lld),le(attributes/geotile,%lld))"),
        Lower, Upper);

    TSharedRef<FString> Cursor = MakeShared<FString>();
    TSharedRef<TFunction<void()>> FetchPage = MakeShared<TFunction<void()>>();

    *FetchPage = [this, Filter, Cursor, OnPageReceived, OnCompleted, FetchPage]() -> void
    {
        const FString BaseRequestURL = BaseUrl
            + TEXT("/api/2/search/things?filter=") + Filter
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
    const int32 ShiftBits = 2 * (MaxZoom - TileZoom);
    OutLower = QuadKey << ShiftBits;
    OutUpper = (QuadKey + 1) << ShiftBits;
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
