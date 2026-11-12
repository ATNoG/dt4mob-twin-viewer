/**
 * @file
 * @brief Implements FDittoEnvelopeParser: turns a Ditto protocol message into thingId, path and value.
 * @ingroup backend
 */
#include "Services/Backend/DittoEnvelopeParser.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"

bool FDittoEnvelopeParser::ParseEnvelope(const FString& RawMessage,
	FString& OutThingId, FString& OutPath, FString& OutValueJson) const
{
	TSharedPtr<FJsonObject> Root;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RawMessage);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return false;
	}

	// ---- topic -> thingId ----
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

	// ---- value -> JSON string ----
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
