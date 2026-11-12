/**
 * @file
 * @brief Declares IBackendEnvelopeParser, the interface that decodes live update messages.
 * @ingroup backend
 */
#pragma once

#include "CoreMinimal.h"

/**
 * @brief Abstract contract for decoding one backend live-update message into the generic
 *        (thingId, path, value) triple UEntityUpdateDaemon routes on.
 *
 * UDittoEnvelopeParser is the only place Ditto's envelope shape (topic/path/value, the
 * "<namespace>/<thingName>/things/twin/events/<action>" topic layout) is known; everything
 * else in UEntityUpdateDaemon (registration, dispatch, stream-health tracking) is generic.
 * @ingroup backend
 */
class DT4MOB_API IBackendEnvelopeParser
{
public:
	virtual ~IBackendEnvelopeParser() = default;

	/**
	 * @brief Parses one raw live-update message. Returns true on success.
	 *
	 * Fills OutThingId, OutPath, and OutValueJson (the changed value, serialised back to a
	 * JSON string) the same way regardless of which concrete backend produced the message.
	 *
	 * @param RawMessage   Text frame received from the WebSocket.
	 * @param OutThingId   Receives the id of the changed thing.
	 * @param OutPath      Receives the changed path inside the thing, e.g. "/attributes/location".
	 * @param OutValueJson Receives the new value as a JSON string.
	 * @return True if the message is a thing event that could be parsed.
	 */
	virtual bool ParseEnvelope(const FString& RawMessage,
		FString& OutThingId, FString& OutPath, FString& OutValueJson) const = 0;
};
