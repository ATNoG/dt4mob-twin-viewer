/**
 * @file
 * @brief Declares FDittoEnvelopeParser, the Ditto protocol implementation of IBackendEnvelopeParser.
 * @ingroup backend
 */
#pragma once

#include "CoreMinimal.h"
#include "Services/Backend/IBackendEnvelopeParser.h"

/**
 * @brief Decodes Ditto's WebSocket envelope:
 *   {"topic": "<namespace>/<thingName>/things/twin/events/<action>", "path": "...", "value": ...}
 * into (thingId = "<namespace>:<thingName>", path, valueJson).
 * @ingroup backend
 */
class FDittoEnvelopeParser : public IBackendEnvelopeParser
{
public:
	virtual bool ParseEnvelope(const FString& RawMessage,
		FString& OutThingId, FString& OutPath, FString& OutValueJson) const override;
};
