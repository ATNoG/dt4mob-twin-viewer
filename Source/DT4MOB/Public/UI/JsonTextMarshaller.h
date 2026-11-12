/**
 * @file
 * @brief Declares FJsonToken, FJsonTextMarshaller and EJsonTokenType.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "Framework/Text/ITextLayoutMarshaller.h"
#include "Framework/Text/TextLayout.h"
#include "Styling/SlateTypes.h"

/** @brief Kind of a JSON token, used to pick its colour. @ingroup ui */
enum class EJsonTokenType : uint8
{
    Key,          ///< A string followed by a colon.
    StringValue,  ///< Any other string.
    Number,       ///< A number literal.
    Bool,         ///< true or false.
    Null,         ///< null.
    Punctuation,  ///< Braces, brackets, commas, colons and any other single character.
    Whitespace,   ///< A run of whitespace.
};

/** @brief One token found on a line of JSON text. @ingroup ui */
struct FJsonToken
{
    int32 Start;          ///< Index of the first character in the line.
    int32 Length;         ///< Number of characters.
    EJsonTokenType Type;  ///< Token kind.
};

/**
 * @brief Slate text marshaller that shows JSON with syntax colouring in a monospace font.
 *
 * Each line is split into tokens by a small hand-written tokenizer (not a full parser), and
 * each token becomes a text run styled by its type. Used by SJsonViewer.
 * @ingroup ui
 */
class DT4MOB_API FJsonTextMarshaller : public ITextLayoutMarshaller
{
public:
    /**
     * @brief Creates a marshaller.
     * @return Shared reference to the new marshaller.
     */
    static TSharedRef<FJsonTextMarshaller> Create();

    /**
     * @brief Tokenizes the text and fills the layout with coloured runs, one line at a time.
     * @param SourceString     JSON text to show.
     * @param TargetTextLayout Layout to fill.
     */
    virtual void SetText(const FString& SourceString, FTextLayout& TargetTextLayout) override;

    /**
     * @brief Returns the text last passed to SetText().
     * @param TargetString     Receives the text.
     * @param SourceTextLayout Layout (not read).
     */
    virtual void GetText(FString& TargetString, const FTextLayout& SourceTextLayout) override;

    /** @brief The text is static, so no live update is needed. @return Always false. */
    virtual bool RequiresLiveUpdate() const override { return false; }

    /** @brief Marks the marshaller dirty so Slate calls SetText() again. */
    virtual void MakeDirty() override { bDirty = true; }

    /** @brief Clears the dirty flag. */
    virtual void ClearDirty() override { bDirty = false; }

    /** @brief Reports the dirty flag. @return True if MakeDirty() was called since the last ClearDirty(). */
    virtual bool IsDirty() const override { return bDirty; }

private:
    FString CachedText;      ///< Text from the last SetText().
    FSlateFontInfo MonoFont; ///< Engine NotoMono at size 11, or the default font if it is missing.
    bool bDirty = false;     ///< Dirty flag for Slate.

    /** @brief Loads the monospace font. Use Create(). */
    FJsonTextMarshaller();

    /**
     * @brief Splits one line of JSON text into tokens.
     * @param Line Text of the line.
     * @return Tokens in order; together they cover the whole line.
     */
    static TArray<FJsonToken> TokenizeLine(const FString& Line);

    /**
     * @brief Builds the text style for a token type.
     * @param Type Token type.
     * @return MonoFont with the colour for the type.
     */
    FTextBlockStyle StyleForType(EJsonTokenType Type) const;

    /**
     * @brief Returns the colour used for a token type.
     * @param Type Token type.
     * @return The colour.
     */
    static FLinearColor ColorForType(EJsonTokenType Type);
};
