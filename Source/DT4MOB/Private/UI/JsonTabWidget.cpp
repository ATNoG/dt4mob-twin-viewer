#include "UI/JsonTabWidget.h"
#include "Entities/TempUIActor.h"
#include "Components/MultiLineEditableTextBox.h"

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
    JsonTextBox->SetText(FText::FromString(Json));
}

void UJsonTabWidget::HandleEntityDataChanged()
{
    Refresh();
}
