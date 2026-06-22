#include "UI/JsonTextMarshaller.h"
#include "Framework/Text/SlateTextRun.h"
#include "Styling/CoreStyle.h"
#include "Engine/Font.h"

FJsonTextMarshaller::FJsonTextMarshaller()
{
    UFont* FontAsset = FindObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/NotoMono.NotoMono"));
    if (!FontAsset)
        FontAsset = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/NotoMono.NotoMono"));

    if (FontAsset)
        MonoFont = FSlateFontInfo(static_cast<UObject*>(FontAsset), 11, TEXT("Regular"));
    else
        MonoFont = FCoreStyle::GetDefaultFontStyle("Regular", 11);
}

TSharedRef<FJsonTextMarshaller> FJsonTextMarshaller::Create()
{
    return MakeShareable(new FJsonTextMarshaller());
}

void FJsonTextMarshaller::GetText(FString& TargetString, const FTextLayout& SourceTextLayout)
{
    TargetString = CachedText;
}

void FJsonTextMarshaller::SetText(const FString& SourceString, FTextLayout& TargetTextLayout)
{
    CachedText = SourceString;
    TargetTextLayout.ClearLines();

    TArray<FString> Lines;
    SourceString.ParseIntoArrayLines(Lines, false);

    TArray<FTextLayout::FNewLineData> LinesToAdd;
    LinesToAdd.Reserve(Lines.Num());

    for (const FString& Line : Lines)
    {
        TSharedRef<FString> LineText = MakeShared<FString>(Line);
        TArray<TSharedRef<IRun>> Runs;

        TArray<FJsonToken> Tokens = TokenizeLine(Line);

        if (Tokens.IsEmpty())
        {
            FRunInfo RunInfo;
            Runs.Add(FSlateTextRun::Create(RunInfo, LineText, StyleForType(EJsonTokenType::Whitespace), FTextRange(0, 0)));
        }
        else
        {
            for (const FJsonToken& Token : Tokens)
            {
                FRunInfo RunInfo;
                Runs.Add(FSlateTextRun::Create(RunInfo, LineText, StyleForType(Token.Type), FTextRange(Token.Start, Token.Start + Token.Length)));
            }
        }

        LinesToAdd.Emplace(MoveTemp(LineText), MoveTemp(Runs));
    }

    TargetTextLayout.AddLines(LinesToAdd);
}

TArray<FJsonToken> FJsonTextMarshaller::TokenizeLine(const FString& Line)
{
    TArray<FJsonToken> Tokens;
    const int32 Len = Line.Len();
    int32 i = 0;

    while (i < Len)
    {
        const TCHAR C = Line[i];

        if (FChar::IsWhitespace(C))
        {
            const int32 Start = i;
            while (i < Len && FChar::IsWhitespace(Line[i])) i++;
            Tokens.Add({ Start, i - Start, EJsonTokenType::Whitespace });
            continue;
        }

        if (C == '"')
        {
            const int32 Start = i++;
            while (i < Len)
            {
                if (Line[i] == '\\') { i += 2; continue; }
                if (Line[i] == '"') { i++; break; }
                i++;
            }
            // Look ahead past whitespace for ':' to classify as key vs string value
            int32 j = i;
            while (j < Len && FChar::IsWhitespace(Line[j])) j++;
            const EJsonTokenType Type = (j < Len && Line[j] == ':') ? EJsonTokenType::Key : EJsonTokenType::StringValue;
            Tokens.Add({ Start, i - Start, Type });
            continue;
        }

        if (FChar::IsDigit(C) || (C == '-' && i + 1 < Len && FChar::IsDigit(Line[i + 1])))
        {
            const int32 Start = i++;
            while (i < Len && (FChar::IsDigit(Line[i]) || Line[i] == '.' || Line[i] == 'e' || Line[i] == 'E' || Line[i] == '+' || Line[i] == '-'))
                i++;
            Tokens.Add({ Start, i - Start, EJsonTokenType::Number });
            continue;
        }

        if (Line.Mid(i, 4) == TEXT("true"))  { Tokens.Add({ i, 4, EJsonTokenType::Bool }); i += 4; continue; }
        if (Line.Mid(i, 5) == TEXT("false")) { Tokens.Add({ i, 5, EJsonTokenType::Bool }); i += 5; continue; }
        if (Line.Mid(i, 4) == TEXT("null"))  { Tokens.Add({ i, 4, EJsonTokenType::Null }); i += 4; continue; }

        Tokens.Add({ i, 1, EJsonTokenType::Punctuation });
        i++;
    }

    return Tokens;
}

FLinearColor FJsonTextMarshaller::ColorForType(EJsonTokenType Type)
{
    switch (Type)
    {
    case EJsonTokenType::Key:          return FLinearColor(0.612f, 0.863f, 0.996f); // #9CDCFE light blue
    case EJsonTokenType::StringValue:  return FLinearColor(0.808f, 0.569f, 0.471f); // #CE9178 orange
    case EJsonTokenType::Number:       return FLinearColor(0.710f, 0.808f, 0.659f); // #B5CEA8 green
    case EJsonTokenType::Bool:         return FLinearColor(0.337f, 0.612f, 0.839f); // #569CD6 blue
    case EJsonTokenType::Null:         return FLinearColor(0.773f, 0.525f, 0.753f); // #C586C0 purple
    default:                           return FLinearColor(0.831f, 0.831f, 0.831f); // #D4D4D4 light gray
    }
}

FTextBlockStyle FJsonTextMarshaller::StyleForType(EJsonTokenType Type) const
{
    FTextBlockStyle Style = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");
    Style.Font = MonoFont;
    Style.ColorAndOpacity = FSlateColor(ColorForType(Type));
    Style.ShadowOffset = FVector2D::ZeroVector;
    Style.ShadowColorAndOpacity = FLinearColor::Transparent;
    return Style;
}
