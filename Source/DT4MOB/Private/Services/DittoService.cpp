// Fill out your copyright notice in the Description page of Project Settings.

/** @file DittoService.cpp
 *  @brief Implementation of UDittoService. All logic documentation is in the header.
 */
#include "Services/DittoService.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Http.h"
#include "JsonObjectConverter.h"
#include "JsonUtilities.h"

void UDittoService::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);
    Http = &FHttpModule::Get();

    const FString SecretsFile = FPaths::ProjectConfigDir() / TEXT("Secrets.ini");
    GConfig->LoadFile(SecretsFile);
    FString Host;
    GConfig->GetString(TEXT("Ditto"), TEXT("Username"), Username, SecretsFile);
    GConfig->GetString(TEXT("Ditto"), TEXT("Password"), Password, SecretsFile);
    GConfig->GetString(TEXT("Ditto"), TEXT("Host"),     Host,     SecretsFile);
    BaseUrl = TEXT("http://") + Host + TEXT("/api");

    if (Username.IsEmpty() || Password.IsEmpty() || Host.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("DittoService: one or more values missing from Config/Secrets.ini"));
    }

    UE_LOG(LogTemp, Log, TEXT("DittoService initialized"));
}

void UDittoService::GetAllThings(
    TFunction<void(const TArray<TSharedPtr<FJsonObject>> &)> OnPageReceived,
    TFunction<void()> OnCompleted)
{
    TSharedRef<FString> Cursor = MakeShared<FString>();
    TSharedRef<TFunction<void()>> FetchPage = MakeShared<TFunction<void()>>();

    *FetchPage = [this, Cursor, OnPageReceived, OnCompleted, FetchPage]() -> void
    {
        const FString BaseRequestURL = BaseUrl + "/2/search/things?filter=ge(thingId,'muro-talude')&option=size(50)";

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
        SetCommonHeaders(Request);

        if (!Cursor->IsEmpty())
        {
            Request->SetURL(BaseRequestURL + TEXT(",cursor(") + *Cursor + TEXT(")"));
        }
        else
        {
            Request->SetURL(BaseRequestURL);
        }

        UE_LOG(LogTemp, Log, TEXT("Requesting URL: %s"), *Request->GetURL());
        Request->SetVerb("GET");

        Request->OnProcessRequestComplete().BindLambda(
            [Cursor, OnPageReceived, OnCompleted, FetchPage](FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful)
            {
                if (!bWasSuccessful || !ResponsePtr.IsValid())
                {
                    UE_LOG(LogTemp, Error, TEXT("HTTP request failed"));
                    if (OnCompleted)
                    {
                        OnCompleted();
                    }
                    return;
                }

                FString ResponseString = ResponsePtr->GetContentAsString();

                TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);

                if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
                {
                    UE_LOG(LogTemp, Error, TEXT("JSON parse failed"));
                    if (OnCompleted)
                    {
                        OnCompleted();
                    }
                    return;
                }

                // ---- Extract items ----
                TArray<TSharedPtr<FJsonObject>> PageItems;

                const TArray<TSharedPtr<FJsonValue>> *ItemsArray;
                if (JsonObject->TryGetArrayField(TEXT("items"), ItemsArray))
                {
                    PageItems.Reserve(ItemsArray->Num());

                    for (const TSharedPtr<FJsonValue> &Item : *ItemsArray)
                    {
                        TSharedPtr<FJsonObject> Obj = Item->AsObject();
                        if (Obj.IsValid())
                        {
                            PageItems.Add(Obj);
                        }
                    }
                }

                // ---- Send page ----
                if (OnPageReceived)
                {
                    OnPageReceived(PageItems);
                }

                // ---- Handle pagination ----
                FString NewCursor;
                if (JsonObject->TryGetStringField(TEXT("cursor"), NewCursor))
                {
                    *Cursor = NewCursor;
                    (*FetchPage)();
                }
                else
                {
                    if (OnCompleted)
                    {
                        OnCompleted();
                    }
                }
            });

        Request->ProcessRequest();
    };

    (*FetchPage)();
}

void UDittoService::SetCommonHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request)
{
    Request->SetHeader("Authorization", "Basic " + FBase64::Encode(Username + ":" + Password));
}

void UDittoService::GetThingsByGeotile(
    double Lat,
    double Lng,
    int32 TileZoom,
    TFunction<void(const TArray<TSharedPtr<FJsonObject>> &)> OnPageReceived,
    TFunction<void()> OnCompleted)
{
    int64 Lower, Upper;
    GetTileBounds(Lat, Lng, TileZoom, Lower, Upper);

    const FString Filter = FString::Printf(
        TEXT("and(ge(attributes/geotile,%lld),le(attributes/geotile,%lld))"),
        Lower, Upper);

    UE_LOG(LogTemp, Log, TEXT("GetThingsByGeotile: zoom=%d lat=%.6f lng=%.6f bounds=[%lld,%lld)"),
        TileZoom, Lat, Lng, Lower, Upper);

    TSharedRef<FString> Cursor = MakeShared<FString>();
    TSharedRef<TFunction<void()>> FetchPage = MakeShared<TFunction<void()>>();

    *FetchPage = [this, Filter, Cursor, OnPageReceived, OnCompleted, FetchPage]() -> void
    {
        const FString BaseRequestURL = BaseUrl
            + TEXT("/2/search/things?filter=") + Filter
            + TEXT("&option=size(200)");

        TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Http->CreateRequest();
        SetCommonHeaders(Request);

        if (!Cursor->IsEmpty())
        {
            Request->SetURL(BaseRequestURL + TEXT(",cursor(") + *Cursor + TEXT(")"));
        }
        else
        {
            Request->SetURL(BaseRequestURL);
        }

        Request->SetVerb("GET");

        Request->OnProcessRequestComplete().BindLambda(
            [Cursor, OnPageReceived, OnCompleted, FetchPage](FHttpRequestPtr RequestPtr, FHttpResponsePtr ResponsePtr, bool bWasSuccessful)
            {
                if (!bWasSuccessful || !ResponsePtr.IsValid())
                {
                    UE_LOG(LogTemp, Error, TEXT("GetThingsByGeotile: HTTP request failed"));
                    if (OnCompleted) OnCompleted();
                    return;
                }

                TSharedPtr<FJsonObject> JsonObject;
                TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponsePtr->GetContentAsString());

                if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
                {
                    UE_LOG(LogTemp, Error, TEXT("GetThingsByGeotile: JSON parse failed"));
                    if (OnCompleted) OnCompleted();
                    return;
                }

                TArray<TSharedPtr<FJsonObject>> PageItems;
                const TArray<TSharedPtr<FJsonValue>> *ItemsArray;
                if (JsonObject->TryGetArrayField(TEXT("items"), ItemsArray))
                {
                    PageItems.Reserve(ItemsArray->Num());
                    for (const TSharedPtr<FJsonValue> &Item : *ItemsArray)
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

        Request->ProcessRequest();
    };

    (*FetchPage)();
}

int64 UDittoService::GetQuadkey(double Lat, double Lng, int32 Zoom)
{
    const int64 TileCount = int64(1) << Zoom;
    int64 X = int64((Lng + 180.0) / 360.0 * double(TileCount));
    const double LatRad = FMath::DegreesToRadians(Lat);
    int64 Y = int64(
        (1.0 - FMath::Loge(FMath::Tan(LatRad) + 1.0 / FMath::Cos(LatRad)) / PI)
        / 2.0 * double(TileCount));

    X = FMath::Clamp(X, int64(0), TileCount - 1);
    Y = FMath::Clamp(Y, int64(0), TileCount - 1);

    int64 Quadkey = 0;
    for (int32 I = Zoom; I > 0; --I)
    {
        const int64 XBit = (X >> I) & 1;
        const int64 YBit = (Y >> I) & 1;
        Quadkey = (Quadkey << 2) | (YBit << 1) | XBit;
    }
    return Quadkey;
}

void UDittoService::GetTileBounds(double Lat, double Lng, int32 TileZoom, int64 &OutLower, int64 &OutUpper, int32 MaxZoom)
{
    const int64 TileQk = GetQuadkey(Lat, Lng, TileZoom);
    const int32 ShiftBits = 2 * (MaxZoom - TileZoom);
    OutLower = TileQk << ShiftBits;
    OutUpper = (TileQk + 1) << ShiftBits;
}

int32 UDittoService::AltitudeToZoomLevel(double AltitudeMeters)
{
    if (AltitudeMeters <= 1.0)
    {
        return MaxTileZoom;
    }
    const double Z = FMath::Log2(10019000.0 / AltitudeMeters);
    return FMath::Clamp(FMath::RoundToInt(Z), 0, MaxTileZoom);
}