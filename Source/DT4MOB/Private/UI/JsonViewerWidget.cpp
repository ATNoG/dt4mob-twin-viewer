#include "UI/JsonViewerWidget.h"
#include "UI/SJsonViewer.h"

TSharedRef<SWidget> UJsonViewerWidget::RebuildWidget()
{
    SAssignNew(MyViewer, SJsonViewer);
    return MyViewer.ToSharedRef();
}

void UJsonViewerWidget::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    MyViewer.Reset();
}

void UJsonViewerWidget::SetJsonText(const FString& Json)
{
    if (MyViewer.IsValid())
        MyViewer->SetJsonText(Json);
}

void UJsonViewerWidget::ScrollToStart()
{
    if (MyViewer.IsValid())
        MyViewer->ScrollToStart();
}
