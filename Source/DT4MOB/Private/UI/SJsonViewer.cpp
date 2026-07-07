#include "UI/SJsonViewer.h"
#include "UI/JsonTextMarshaller.h"
#include "Widgets/Text/SMultiLineEditableText.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/CoreStyle.h"
#include "Styling/StyleDefaults.h"

void SJsonViewer::Construct(const FArguments& InArgs)
{
    Marshaller = FJsonTextMarshaller::Create();

    const FTextBlockStyle& BaseStyle = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("NormalText");

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(FStyleDefaults::GetNoBrush())
        .ColorAndOpacity(FLinearColor::White)
        .Padding(FMargin(10.f, 8.f))
        [
            SAssignNew(ScrollBox, SScrollBox)
            .Orientation(Orient_Vertical)
            + SScrollBox::Slot()
            .HAlign(HAlign_Fill)
            [
                SAssignNew(TextWidget, SMultiLineEditableText)
                .IsReadOnly(true)
                .AutoWrapText(true)
                .Marshaller(Marshaller)
                .TextStyle(&BaseStyle)
            ]
        ]
    ];
}

void SJsonViewer::SetJsonText(const FString& Json)
{
    if (TextWidget.IsValid())
        TextWidget->SetText(FText::FromString(Json));
}

void SJsonViewer::ScrollToStart()
{
    if (ScrollBox.IsValid())
        ScrollBox->ScrollToStart();
}
