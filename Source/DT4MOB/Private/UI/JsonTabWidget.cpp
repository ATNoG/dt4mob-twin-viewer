#include "UI/JsonTabWidget.h"
#include "UI/JsonViewerWidget.h"
#include "Entities/TempUIActor.h"

void UJsonTabWidget::NativeDestruct()
{
    SetBoundActor(nullptr);
    Super::NativeDestruct();
}

void UJsonTabWidget::SetBoundActor(ATempUIActor* Actor)
{
    if (IsValid(BoundActor))
        BoundActor->OnEntityDataChanged.RemoveDynamic(this, &UJsonTabWidget::HandleEntityDataChanged);

    BoundActor = Actor;

    if (IsValid(BoundActor))
        BoundActor->OnEntityDataChanged.AddDynamic(this, &UJsonTabWidget::HandleEntityDataChanged);

    Refresh();
}

void UJsonTabWidget::Refresh()
{
    if (!JsonTextBox)
        return;

    const FString Json = IsValid(BoundActor) ? BoundActor->GetJsonString() : FString();
    JsonTextBox->SetJsonText(Json);
    JsonTextBox->ScrollToStart();
}

void UJsonTabWidget::HandleEntityDataChanged()
{
    Refresh();
}
