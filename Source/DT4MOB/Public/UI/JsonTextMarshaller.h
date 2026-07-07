#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/ITextLayoutMarshaller.h"
#include "Framework/Text/TextLayout.h"
#include "Styling/SlateTypes.h"

enum class EJsonTokenType : uint8
{
    Key,
    StringValue,
    Number,
    Bool,
    Null,
    Punctuation,
    Whitespace,
};

struct FJsonToken
{
    int32 Start;
    int32 Length;
    EJsonTokenType Type;
};

class DT4MOB_API FJsonTextMarshaller : public ITextLayoutMarshaller
{
public:
    static TSharedRef<FJsonTextMarshaller> Create();

    virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout) override;
    virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;
    virtual bool RequiresLiveUpdate() const override { return false; }
    virtual void MakeDirty() override { bDirty = true; }
    virtual void ClearDirty() override { bDirty = false; }
    virtual bool IsDirty() const override { return bDirty; }

private:
    FString CachedText;
    FSlateFontInfo MonoFont;
    bool bDirty = false;

    FJsonTextMarshaller();

    static TArray<FJsonToken> TokenizeLine(const FString& Line);
    FTextBlockStyle StyleForType(EJsonTokenType Type) const;
    static FLinearColor ColorForType(EJsonTokenType Type);
};
