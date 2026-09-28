#pragma once

#include "CoreMinimal.h"

/**
 * @brief Abstract contract for decoding one backend live-update message into the generic
 *        (thingId, path, value) triple UEntityUpdateDaemon routes on.
 *
 * UDittoEnvelopeParser is the only place Ditto's envelope shape (topic/path/value, the
 * "<namespace>/<thingName>/things/twin/events/<action>" topic layout) is known; everything
 * else in UEntityUpdateDaemon (registration, dispatch, stream-health tracking) is generic.
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
	 */
	virtual bool ParseEnvelope(const FString& RawMessage,
		FString& OutThingId, FString& OutPath, FString& OutValueJson) const = 0;
};
