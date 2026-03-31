// Fill out your copyright notice in the Description page of Project Settings.

/** @file EntityUpdateDaemon.cpp
 *  @brief Implementation of UEntityUpdateDaemon. All logic documentation is in the header.
 */
#include "Services/EntityUpdateDaemon.h"
#include "Services/WSService.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Engine/GameInstance.h"

// ============================================================
//  Subsystem lifecycle
// ============================================================

void UEntityUpdateDaemon::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    Collection.InitializeDependency<UWSService>();

    UGameInstance *GI = GetGameInstance();
    if (!GI)
        return;

    WSService = GI->GetSubsystem<UWSService>();
    if (!WSService)
    {
        UE_LOG(LogTemp, Error, TEXT("EntityUpdateDaemon: UWSService subsystem not found"));
        return;
    }

    WSService->OnConnected.AddDynamic(this, &UEntityUpdateDaemon::HandleSocketConnected);
    WSService->OnMessage.AddDynamic(this, &UEntityUpdateDaemon::HandleSocketMessage);
    WSService->OnError.AddDynamic(this, &UEntityUpdateDaemon::HandleSocketError);
    WSService->OnClosed.AddDynamic(this, &UEntityUpdateDaemon::HandleSocketClosed);

    UE_LOG(LogTemp, Log, TEXT("EntityUpdateDaemon: Initialized"));
}

void UEntityUpdateDaemon::Deinitialize()
{
    if (WSService)
    {
        WSService->OnConnected.RemoveDynamic(this, &UEntityUpdateDaemon::HandleSocketConnected);
        WSService->OnMessage.RemoveDynamic(this, &UEntityUpdateDaemon::HandleSocketMessage);
        WSService->OnError.RemoveDynamic(this, &UEntityUpdateDaemon::HandleSocketError);
        WSService->OnClosed.RemoveDynamic(this, &UEntityUpdateDaemon::HandleSocketClosed);

        if (bConnectionRequested)
        {
            WSService->Disconnect();
        }

        WSService = nullptr;
    }

    EntityDelegates.Empty();
    UE_LOG(LogTemp, Log, TEXT("EntityUpdateDaemon: Deinitialized"));

    Super::Deinitialize();
}

// ============================================================
//  Connection API
// ============================================================

void UEntityUpdateDaemon::StartConnection(const FString &InUrl,
                                          const FString &InAuthHeader,
                                          const FString &InStartMessage,
                                          bool bInAutoReconnect,
                                          float InReconnectDelaySeconds)
{
    if (!WSService)
    {
        UE_LOG(LogTemp, Error, TEXT("EntityUpdateDaemon::StartConnection: WSService is null"));
        return;
    }

    Url = InUrl;
    AuthHeader = InAuthHeader;
    StartMessage = InStartMessage;
    bAutoReconnect = bInAutoReconnect;
    ReconnectDelaySeconds = InReconnectDelaySeconds;
    bConnectionRequested = true;

    WSService->Connect(Url, AuthHeader, StartMessage, bAutoReconnect, ReconnectDelaySeconds);
    UE_LOG(LogTemp, Log, TEXT("EntityUpdateDaemon: StartConnection -> %s"), *Url);
}

void UEntityUpdateDaemon::StopConnection()
{
    bConnectionRequested = false;

    if (WSService)
    {
        WSService->Disconnect();
    }

    UE_LOG(LogTemp, Log, TEXT("EntityUpdateDaemon: StopConnection"));
}

// ============================================================
//  Entity registration
// ============================================================

void UEntityUpdateDaemon::RegisterEntity(const FString &ThingId, FOnEntityUpdated &Delegate)
{
    TArray<FOnEntityUpdated *> &Arr = EntityDelegates.FindOrAdd(ThingId);
    if (!Arr.Contains(&Delegate))
    {
        Arr.Add(&Delegate);
        UE_LOG(LogTemp, Log, TEXT("EntityUpdateDaemon: Registered entity '%s' (total for key: %d)"),
               *ThingId, Arr.Num());
    }
}

void UEntityUpdateDaemon::UnregisterEntity(const FString &ThingId, FOnEntityUpdated &Delegate)
{
    if (TArray<FOnEntityUpdated *> *Arr = EntityDelegates.Find(ThingId))
    {
        Arr->Remove(&Delegate);
        if (Arr->Num() == 0)
        {
            EntityDelegates.Remove(ThingId);
        }
        UE_LOG(LogTemp, Log, TEXT("EntityUpdateDaemon: Unregistered entity '%s'"), *ThingId);
    }
}

// ============================================================
//  Socket event handlers
// ============================================================

void UEntityUpdateDaemon::HandleSocketConnected()
{
    UE_LOG(LogTemp, Log, TEXT("EntityUpdateDaemon: WebSocket connected"));
    OnSocketConnected.Broadcast();
}

void UEntityUpdateDaemon::HandleSocketMessage(const FString &Message)
{
    FString ThingId, Path, ValueJson;
    if (ParseDittoMessage(Message, ThingId, Path, ValueJson))
    {
        DispatchUpdate(ThingId, Path, ValueJson);
    }
}

void UEntityUpdateDaemon::HandleSocketError(const FString &Error)
{
    UE_LOG(LogTemp, Warning, TEXT("EntityUpdateDaemon: WebSocket error: %s"), *Error);
    OnSocketError.Broadcast(Error);
}

void UEntityUpdateDaemon::HandleSocketClosed(int32 StatusCode, const FString &Reason)
{
    UE_LOG(LogTemp, Log, TEXT("EntityUpdateDaemon: WebSocket closed [%d] %s"), StatusCode, *Reason);
    OnSocketClosed.Broadcast(StatusCode, Reason);
}

// ============================================================
//  Ditto parsing
//
//  Envelope format:
//    {
//      "topic": "tolls/toll-1/things/twin/events/modified",
//      "path":  "/features/lidar-outsight/properties/1",
//      "value": { ... }
//    }
//
//  Topic layout: <namespace>/<thingName>/things/twin/events/<action>
//  ThingId:      <namespace>:<thingName>   (e.g. "tolls:toll-1")
// ============================================================

bool UEntityUpdateDaemon::ParseDittoMessage(const FString &RawMessage,
                                            FString &OutThingId,
                                            FString &OutPath,
                                            FString &OutValueJson) const
{
    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RawMessage);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        return false;
    }

    // ---- topic → thingId ----
    FString Topic;
    if (!Root->TryGetStringField(TEXT("topic"), Topic))
    {
        return false;
    }

    // "tolls/toll-1/things/twin/events/modified"
    //  [0]      [1]   [2]   [3]  [4]    [5]
    TArray<FString> TopicParts;
    Topic.ParseIntoArray(TopicParts, TEXT("/"), /*CullEmpty=*/true);
    if (TopicParts.Num() < 2)
    {
        return false;
    }

    OutThingId = TopicParts[0] + TEXT(":") + TopicParts[1];

    // ---- path ----
    if (!Root->TryGetStringField(TEXT("path"), OutPath))
    {
        return false;
    }

    // ---- value → JSON string ----
    const TSharedPtr<FJsonValue> ValueField = Root->TryGetField(TEXT("value"));
    if (!ValueField.IsValid())
    {
        OutValueJson = TEXT("{}");
        return true; // path-only update is still valid
    }

    if (ValueField->Type == EJson::Object)
    {
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutValueJson);
        FJsonSerializer::Serialize(ValueField->AsObject().ToSharedRef(), Writer);
    }
    else if (ValueField->Type == EJson::String)
    {
        // Preserve JSON encoding so the receiver can parse it back as a JSON string
        FString Escaped = ValueField->AsString()
            .Replace(TEXT("\\"), TEXT("\\\\"))
            .Replace(TEXT("\""), TEXT("\\\""));
        OutValueJson = TEXT("\"") + Escaped + TEXT("\"");
    }
    else if (ValueField->Type == EJson::Number)
    {
        OutValueJson = FString::SanitizeFloat(ValueField->AsNumber());
    }
    else if (ValueField->Type == EJson::Boolean)
    {
        OutValueJson = ValueField->AsBool() ? TEXT("true") : TEXT("false");
    }
    else
    {
        OutValueJson = TEXT("{}");
    }

    return true;
}

// ============================================================
//  Dispatch
// ============================================================

void UEntityUpdateDaemon::DispatchUpdate(const FString &ThingId,
                                         const FString &Path,
                                         const FString &ValueJson)
{
    TArray<FOnEntityUpdated *> *Arr = EntityDelegates.Find(ThingId);
    if (!Arr)
    {
        return; // no actor registered for this thingId
    }

    for (FOnEntityUpdated *Delegate : *Arr)
    {
        if (Delegate && Delegate->IsBound())
        {
            Delegate->Broadcast(Path, ValueJson);
        }
    }
}
