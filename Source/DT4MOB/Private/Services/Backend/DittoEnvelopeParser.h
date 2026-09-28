#pragma once

#include "CoreMinimal.h"
#include "Services/Backend/IBackendEnvelopeParser.h"

/**
 * @brief Decodes Ditto's WebSocket envelope:
 *   {"topic": "<namespace>/<thingName>/things/twin/events/<action>", "path": "...", "value": ...}
 * into (thingId = "<namespace>:<thingName>", path, valueJson).
 */
class FDittoEnvelopeParser : public IBackendEnvelopeParser
{
public:
	virtual bool ParseEnvelope(const FString& RawMessage,
		FString& OutThingId, FString& OutPath, FString& OutValueJson) const override;
};
