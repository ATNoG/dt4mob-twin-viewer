// Fill out your copyright notice in the Description page of Project Settings.

/** @file BackendConnectionService.cpp
 *  @brief Implementation of UBackendConnectionService. All logic documentation is in the header.
 *
 *  Backend contact (authentication, REST requests) is delegated to UDittoRestClient, the one
 *  implementation of IBackendRestClient constructed in Initialize(). This class owns lifecycle,
 *  the DA_DittoSecrets-derived config getters, delegates, and the geotile/quadkey math, and
 *  otherwise just forwards.
 */
#include "Services/BackendConnectionService.h"
#include "Services/DittoSecretsAsset.h"
#include "Services/Backend/DittoRestClient.h"
#include "GeotileUtils.h"

// ─── Lifecycle ───────────────────────────────────────────────────────────────

void UBackendConnectionService::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Only non-credential defaults come from the DataAsset now — actual login happens at
    // runtime via Login(), called from the login screen (see UCredentialStoreService for the
    // persisted-credentials path that skips showing that screen).
    const UDittoSecretsAsset* Secrets = LoadObject<UDittoSecretsAsset>(
        nullptr, TEXT("/Game/Data/DA_DittoSecrets.DA_DittoSecrets"));

    bool bUseOAuth = true;
    FString OAuthClientId;

    if (Secrets)
    {
        DefaultHost    = Secrets->Host;
        bUseHttps      = Secrets->bUseHttps;
        bUseOAuth      = Secrets->bUseOAuth;
        OAuthClientId  = Secrets->OAuthClientId;
        WsStartMessage = Secrets->WsStartMessage;
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("BackendConnectionService: no secrets DataAsset at /Game/Data/DA_DittoSecrets — using engine defaults, host must be supplied via the login screen"));
    }

    UDittoRestClient* Impl = NewObject<UDittoRestClient>(this);
    Impl->Configure(bUseHttps, bUseOAuth, OAuthClientId,
        [this](const FString& AuthHeader) { OnAuthHeaderReady.Broadcast(AuthHeader); });
    RestClientOwner = Impl;
    RestClient = Impl;

    UE_LOG(LogTemp, Log, TEXT("BackendConnectionService initialized — awaiting login (auth=%s)"),
           bUseOAuth ? TEXT("OAuth") : TEXT("Basic"));
}

void UBackendConnectionService::Deinitialize()
{
    if (RestClient)
    {
        RestClient->Shutdown();
    }
    RestClient = nullptr;
    RestClientOwner = nullptr;
    Super::Deinitialize();
}

void UBackendConnectionService::Login(const FString& InHost, const FString& InUsername, const FString& InPassword,
    TFunction<void(bool)> OnComplete)
{
    if (RestClient)
    {
        RestClient->Login(InHost, InUsername, InPassword, MoveTemp(OnComplete));
    }
    else if (OnComplete)
    {
        OnComplete(false);
    }
}

void UBackendConnectionService::Logout()
{
    if (RestClient)
    {
        RestClient->Logout();
    }

    UE_LOG(LogTemp, Log, TEXT("BackendConnectionService: logged out"));
    OnLoggedOut.Broadcast();
}

bool UBackendConnectionService::IsAuthenticated() const
{
    return RestClient && RestClient->IsAuthenticated();
}

FString UBackendConnectionService::GetCurrentAuthHeader() const
{
    return RestClient ? RestClient->GetCurrentAuthHeader() : FString();
}

FString UBackendConnectionService::GetHost() const
{
    return RestClient ? RestClient->GetHost() : FString();
}

// ─── API ─────────────────────────────────────────────────────────────────────

void UBackendConnectionService::GetAllThings(
    TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
    TFunction<void()> OnCompleted,
    const FString& Filter)
{
    if (RestClient)
    {
        RestClient->GetAllThings(MoveTemp(OnPageReceived), MoveTemp(OnCompleted), Filter);
    }
    else if (OnCompleted)
    {
        OnCompleted();
    }
}

void UBackendConnectionService::GetThingsByGeotile(
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

void UBackendConnectionService::GetThingsByGeotileBounds(
    int64 Lower,
    int64 Upper,
    TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
    TFunction<void()> OnCompleted)
{
    if (RestClient)
    {
        RestClient->GetThingsByGeotileBounds(Lower, Upper, MoveTemp(OnPageReceived), MoveTemp(OnCompleted));
    }
    else if (OnCompleted)
    {
        OnCompleted();
    }
}

void UBackendConnectionService::GetThingById(
    const FString& ThingId,
    TFunction<void(TSharedPtr<FJsonObject>)> OnComplete)
{
    if (RestClient)
    {
        RestClient->GetThingById(ThingId, MoveTemp(OnComplete));
    }
    else if (OnComplete)
    {
        OnComplete(nullptr);
    }
}

void UBackendConnectionService::PutThing(
    const FString& ThingId,
    TSharedPtr<FJsonObject> Body,
    TFunction<void(bool)> OnComplete)
{
    if (RestClient)
    {
        RestClient->PutThing(ThingId, Body, MoveTemp(OnComplete));
    }
    else if (OnComplete)
    {
        OnComplete(false);
    }
}

// ─── Geo math ────────────────────────────────────────────────────────────────

void UBackendConnectionService::GetTileXY(double Lat, double Lng, int32 Zoom, int64& OutX, int64& OutY)
{
    FGeotileUtils::LatLonToTileXY(Lat, Lng, Zoom, OutX, OutY);
}

int64 UBackendConnectionService::GetQuadkeyFromXY(int64 X, int64 Y, int32 Zoom)
{
    return FGeotileUtils::TileXYToQuadkey(X, Y, Zoom);
}

int64 UBackendConnectionService::GetQuadkey(double Lat, double Lng, int32 Zoom)
{
    return FGeotileUtils::LatLonToGeotile(Lat, Lng, Zoom);
}

void UBackendConnectionService::GetTileBoundsFromKey(int64 QuadKey, int32 TileZoom, int64& OutLower, int64& OutUpper, int32 MaxZoom)
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

void UBackendConnectionService::GetTileBounds(double Lat, double Lng, int32 TileZoom, int64& OutLower, int64& OutUpper, int32 MaxZoom)
{
    FGeotileUtils::GeotileBounds(Lat, Lng, TileZoom, OutLower, OutUpper, MaxZoom);
}

int32 UBackendConnectionService::AltitudeToZoomLevel(double AltitudeMeters)
{
    if (AltitudeMeters <= 1.0) return MaxTileZoom;
    const double Z = FMath::Log2(10019000.0 / AltitudeMeters);
    return FMath::Clamp(FMath::RoundToInt(Z), 0, MaxTileZoom);
}
