/** @file DittoRestClient.cpp
 *  @brief Implementation of UDittoRestClient. Moved verbatim out of BackendConnectionService.cpp when the
 *         IBackendRestClient seam was introduced — see that class's header for behavioural
 *         documentation (this is the same logic, renamed).
 */
#include "Services/Backend/DittoRestClient.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "JsonUtilities.h"
#include "Misc/Base64.h"

// ─── Setup / lifecycle ─────────────────────────────────────────────────────────

void UDittoRestClient::Configure(bool bInUseHttps, bool bInUseOAuth, const FString& InOAuthClientId,
	TFunction<void(const FString&)> InOnAuthHeaderReady)
{
	bUseHttps = bInUseHttps;
	bUseOAuth = bInUseOAuth;
	OAuthClientId = InOAuthClientId;
	OnAuthHeaderReady = MoveTemp(InOnAuthHeaderReady);
	Http = &FHttpModule::Get();
}

void UDittoRestClient::Shutdown()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TokenRefreshTimer);
	}
	Http = nullptr;
}

// ─── Authentication ───────────────────────────────────────────────────────────

void UDittoRestClient::Login(const FString& InHost, const FString& InUsername, const FString& InPassword,
	TFunction<void(bool)> OnComplete)
{
	Host = InHost;
	Username = InUsername;
	Password = InPassword;
	BaseUrl = (bUseHttps ? TEXT("https://") : TEXT("http://")) + Host;
	bCredentialsSet = true;

	UE_LOG(LogTemp, Log, TEXT("DittoRestClient: logging in — user='%s' baseUrl='%s' auth=%s"),
	       *Username, *BaseUrl, bUseOAuth ? TEXT("OAuth") : TEXT("Basic"));

	if (bUseOAuth)
	{
		GetOAuthToken(OnComplete);
	}
	else
	{
		// Basic auth is immediately available.
		if (OnAuthHeaderReady) OnAuthHeaderReady(GetCurrentAuthHeader());
		if (OnComplete) OnComplete(true);
	}
}

void UDittoRestClient::Logout()
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

	UE_LOG(LogTemp, Log, TEXT("DittoRestClient: logged out"));
}

bool UDittoRestClient::IsAuthenticated() const
{
	if (bUseOAuth)
		return !OAuthToken.IsEmpty();

	return bCredentialsSet;
}

void UDittoRestClient::GetOAuthToken(TFunction<void(bool)> OnComplete)
{
	const FString Body = FString::Printf(
		TEXT("client_id=%s&grant_type=password&username=%s&password=%s"),
		*FGenericPlatformHttp::UrlEncode(OAuthClientId),
		*FGenericPlatformHttp::UrlEncode(Username),
		*FGenericPlatformHttp::UrlEncode(Password));

	SendTokenRequest(Body, OnComplete);
}

void UDittoRestClient::RefreshOAuthToken()
{
	if (RefreshToken.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("DittoRestClient: no refresh token available, falling back to full re-auth"));
		GetOAuthToken();
		return;
	}

	const FString Body = FString::Printf(
		TEXT("client_id=%s&grant_type=refresh_token&refresh_token=%s"),
		*FGenericPlatformHttp::UrlEncode(OAuthClientId),
		*FGenericPlatformHttp::UrlEncode(RefreshToken));

	SendTokenRequest(Body);
}

void UDittoRestClient::SendTokenRequest(const FString& Body, TFunction<void(bool)> OnComplete)
{
	if (bAuthInProgress)
	{
		UE_LOG(LogTemp, Log, TEXT("DittoRestClient: token request already in flight, skipping duplicate"));
		return;
	}

	bAuthInProgress = true;

	const FString Url = BaseUrl + TEXT("/auth/realms/dt4mob/protocol/openid-connect/token");
	UE_LOG(LogTemp, Log, TEXT("DittoRestClient: requesting token from %s"), *Url);

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
				UE_LOG(LogTemp, Warning, TEXT("DittoRestClient: token request failed (code %d)"),
				       Response.IsValid() ? Response->GetResponseCode() : -1);
				if (OnComplete) OnComplete(false);
				return;
			}

			TSharedPtr<FJsonObject> Json;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, Json) || !Json.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("DittoRestClient: token response JSON parse failed"));
				if (OnComplete) OnComplete(false);
				return;
			}

			FString AccessToken;
			if (!Json->TryGetStringField(TEXT("access_token"), AccessToken))
			{
				UE_LOG(LogTemp, Warning, TEXT("DittoRestClient: token response missing access_token"));
				if (OnComplete) OnComplete(false);
				return;
			}

			OAuthToken = AccessToken;
			UE_LOG(LogTemp, Log, TEXT("DittoRestClient: access token obtained (length %d)"), OAuthToken.Len());

			FString NewRefreshToken;
			if (Json->TryGetStringField(TEXT("refresh_token"), NewRefreshToken))
			{
				RefreshToken = NewRefreshToken;
				UE_LOG(LogTemp, Log, TEXT("DittoRestClient: refresh token obtained (length %d)"), RefreshToken.Len());
			}

			// Schedule a proactive refresh 30 s before the token expires.
			double ExpiresIn = 300.0;
			Json->TryGetNumberField(TEXT("expires_in"), ExpiresIn);
			const float RefreshDelay = FMath::Max(static_cast<float>(ExpiresIn) - 30.0f, 10.0f);

			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimer(
					TokenRefreshTimer, this, &UDittoRestClient::RefreshOAuthToken, RefreshDelay, false);
				UE_LOG(LogTemp, Log, TEXT("DittoRestClient: token refresh scheduled in %.0f s"), RefreshDelay);
			}

			FlushPendingRequests();
			if (OnAuthHeaderReady) OnAuthHeaderReady(TEXT("Bearer ") + OAuthToken);
			if (OnComplete) OnComplete(true);
		});

	Request->ProcessRequest();
}

void UDittoRestClient::FlushPendingRequests()
{
	if (PendingRequests.IsEmpty()) return;

	UE_LOG(LogTemp, Log, TEXT("DittoRestClient: flushing %d queued request(s)"), PendingRequests.Num());
	TArray<TFunction<void()>> ToFlush = MoveTemp(PendingRequests);
	for (auto& Fn : ToFlush)
	{
		Fn();
	}
}

void UDittoRestClient::SendAuthenticatedRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request)
{
	// Basic auth is ready as soon as Login() has set credentials — no token gating needed.
	if (!bUseOAuth && bCredentialsSet)
	{
		UE_LOG(LogTemp, Verbose, TEXT("DittoRestClient: -> %s %s"), *Request->GetVerb(), *Request->GetURL());
		SetCommonHeaders(Request);
		Request->ProcessRequest();
		return;
	}

	if (bUseOAuth && !OAuthToken.IsEmpty())
	{
		UE_LOG(LogTemp, Verbose, TEXT("DittoRestClient: -> %s %s"), *Request->GetVerb(), *Request->GetURL());
		SetCommonHeaders(Request);
		Request->ProcessRequest();
		return;
	}

	// Token not ready — queue and ensure auth is running.
	FString QueuedUrl = Request->GetURL();
	PendingRequests.Add([this, Request, QueuedUrl]()
	{
		UE_LOG(LogTemp, Verbose, TEXT("DittoRestClient: flushing %s %s"),
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
		UE_LOG(LogTemp, Log, TEXT("DittoRestClient: token not ready, starting auth before queued request"));
		GetOAuthToken();
	}
}

void UDittoRestClient::SetCommonHeaders(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request)
{
	Request->SetHeader(TEXT("Authorization"), GetCurrentAuthHeader());
}

FString UDittoRestClient::GetCurrentAuthHeader() const
{
	if (bUseOAuth)
		return OAuthToken.IsEmpty() ? FString() : TEXT("Bearer ") + OAuthToken;

	return TEXT("Basic ") + FBase64::Encode(Username + TEXT(":") + Password);
}

// ─── API ─────────────────────────────────────────────────────────────────────

void UDittoRestClient::GetAllThings(
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
					UE_LOG(LogTemp, Error, TEXT("DittoRestClient::GetAllThings: HTTP request failed"));
					if (OnCompleted) OnCompleted();
					return;
				}

				const int32 Code = ResponsePtr->GetResponseCode();
				if (Code != 200)
				{
					UE_LOG(LogTemp, Error, TEXT("DittoRestClient::GetAllThings: HTTP %d — %s"),
					       Code, *ResponsePtr->GetContentAsString().Left(512));
					if (OnCompleted) OnCompleted();
					return;
				}

				TSharedPtr<FJsonObject> JsonObject;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponsePtr->GetContentAsString());
				if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
				{
					UE_LOG(LogTemp, Error, TEXT("DittoRestClient::GetAllThings: JSON parse failed"));
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

				UE_LOG(LogTemp, Verbose, TEXT("DittoRestClient: <- GetAllThings page: %d items, cursor=%s"),
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

void UDittoRestClient::GetThingsByGeotileBounds(
	int64 Lower,
	int64 Upper,
	TFunction<void(const TArray<TSharedPtr<FJsonObject>>&)> OnPageReceived,
	TFunction<void()> OnCompleted)
{
	// Upper is the exclusive start of the next tile's range (see UBackendConnectionService::GetTileBoundsFromKey)
	// — use lt, not le, or an entity sitting exactly on a tile boundary gets returned (and spawned)
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
					UE_LOG(LogTemp, Error, TEXT("DittoRestClient::GetThingsByGeotileBounds: HTTP request failed"));
					if (OnCompleted) OnCompleted();
					return;
				}

				TSharedPtr<FJsonObject> JsonObject;
				TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponsePtr->GetContentAsString());
				if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
				{
					UE_LOG(LogTemp, Error, TEXT("DittoRestClient::GetThingsByGeotileBounds: JSON parse failed"));
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

void UDittoRestClient::GetThingById(
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
				UE_LOG(LogTemp, Warning, TEXT("DittoRestClient::GetThingById [%s] failed (code %d)"),
				       *ThingId, Response.IsValid() ? Response->GetResponseCode() : -1);
				if (OnComplete) OnComplete(nullptr);
				return;
			}

			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
			if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("DittoRestClient::GetThingById [%s] JSON parse failed"), *ThingId);
				if (OnComplete) OnComplete(nullptr);
				return;
			}

			if (OnComplete) OnComplete(JsonObject);
		});

	SendAuthenticatedRequest(Request);
}

void UDittoRestClient::PutThing(
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

	UE_LOG(LogTemp, Log, TEXT("DittoRestClient::PutThing [%s]"), *ThingId);

	Request->OnProcessRequestComplete().BindLambda(
		[OnComplete, ThingId](FHttpRequestPtr, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			const bool bSuccess = bWasSuccessful && Response.IsValid()
				&& Response->GetResponseCode() >= 200 && Response->GetResponseCode() < 300;
			UE_LOG(LogTemp, Log, TEXT("DittoRestClient::PutThing [%s] → %d"),
			       *ThingId, Response.IsValid() ? Response->GetResponseCode() : -1);
			if (!bSuccess && Response.IsValid())
			{
				// Ditto's error responses are structured JSON (status/error/message/description)
				// that names the exact rejection reason — e.g. a permission failure disguised as
				// 404 vs. an actually-missing policy look identical at the status-code level.
				UE_LOG(LogTemp, Warning, TEXT("DittoRestClient::PutThing [%s] body: %s"),
				       *ThingId, *Response->GetContentAsString());
			}
			if (OnComplete) OnComplete(bSuccess);
		});

	SendAuthenticatedRequest(Request);
}
