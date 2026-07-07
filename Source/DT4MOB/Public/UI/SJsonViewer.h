#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FJsonTextMarshaller;
class SMultiLineEditableText;
class SScrollBox;

class DT4MOB_API SJsonViewer : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SJsonViewer) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);
    void SetJsonText(const FString& Json);
    void ScrollToStart();

private:
    TSharedPtr<FJsonTextMarshaller> Marshaller;
    TSharedPtr<SMultiLineEditableText> TextWidget;
    TSharedPtr<SScrollBox> ScrollBox;
};
