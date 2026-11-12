/**
 * @file
 * @brief Declares FDittoEventSubscription, the Ditto WebSocket URL and subscribe/unsubscribe commands.
 * @ingroup backend
 */
#pragma once

#include "CoreMinimal.h"
#include "Services/Backend/IBackendEventSubscription.h"

/**
 * @brief Ditto's WebSocket protocol: ws:// or wss:// plus the host plus /ws/2, the "ditto-protocol" subprotocol,
 *        and START-SEND-EVENTS[?filter=...] / STOP-SEND-EVENTS commands.
 * @ingroup backend
 */
class FDittoEventSubscription : public IBackendEventSubscription
{
public:
	virtual FString GetConnectionUrl(const FString& Host, bool bUseHttps) const override
	{
		return (bUseHttps ? TEXT("wss://") : TEXT("ws://")) + Host + TEXT("/ws/2");
	}

	virtual FString GetSocketSubprotocol() const override
	{
		return TEXT("ditto-protocol");
	}

	virtual FString BuildFilteredSubscribeCommand(const FString& Filter) const override
	{
		return TEXT("START-SEND-EVENTS?filter=") + Filter;
	}

	virtual FString BuildUnsubscribeCommand() const override
	{
		return TEXT("STOP-SEND-EVENTS");
	}
};
