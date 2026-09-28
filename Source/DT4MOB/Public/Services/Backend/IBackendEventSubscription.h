#pragma once

#include "CoreMinimal.h"

/**
 * @brief Abstract contract for the live connection's wire protocol: the connection URL,
 *        the WebSocket subprotocol, and how to (un)subscribe to the event stream.
 *
 * UWSService owns one implementation of this (FDittoEventSubscription) and is otherwise
 * unaware of Ditto's specific commands or URL shape; everything else in UWSService
 * (reconnect back-off, delegate broadcasting, generic Connect/Disconnect/SendText) does
 * not vary with the backend and is not behind this interface.
 */
class DT4MOB_API IBackendEventSubscription
{
public:
	virtual ~IBackendEventSubscription() = default;

	/** @brief Builds the WebSocket connection URL for the given host. */
	virtual FString GetConnectionUrl(const FString& Host, bool bUseHttps) const = 0;

	/** @brief The WebSocket subprotocol name to request when opening the socket. */
	virtual FString GetSocketSubprotocol() const = 0;

	/** @brief Builds the command that (re)subscribes with the given non-empty filter. */
	virtual FString BuildFilteredSubscribeCommand(const FString& Filter) const = 0;

	/** @brief Builds the command that cancels the current subscription. */
	virtual FString BuildUnsubscribeCommand() const = 0;
};
