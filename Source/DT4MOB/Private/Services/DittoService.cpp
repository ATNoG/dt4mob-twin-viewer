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
        const FString BaseRequestURL = BaseUrl + "/2/search/things?filter=ge(thingId,'equivia')&option=size(50)";

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