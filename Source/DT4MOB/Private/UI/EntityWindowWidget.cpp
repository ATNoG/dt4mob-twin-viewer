// EntityWindowWidget.cpp
/** @file EntityWindowWidget.cpp
 *  @brief Implementation of UEntityWindowWidget. All logic documentation is in the header.
 */
#include "UI/EntityWindowWidget.h"
#include "Components/TextBlock.h"
#include "Entities/TempUIActor.h"

void UEntityWindowWidget::OnBindData_Implementation(AActor *Actor)
{
    // Unbind from the previous actor
    if (BoundActor)
    {
        BoundActor->OnEntityDataChanged.RemoveDynamic(this, &UEntityWindowWidget::HandleDataChanged);
        BoundActor = nullptr;
    }

    if (!Actor)
        return;

    if (NameText)
    {
        NameText->SetText(FText::FromString(Actor->GetName()));
    }

    if (ATempUIActor *TempActor = Cast<ATempUIActor>(Actor))
    {
        BoundActor = TempActor;
        BoundActor->OnEntityDataChanged.AddDynamic(this, &UEntityWindowWidget::HandleDataChanged);

        if (DataText)
        {
            DataText->SetText(FText::FromString(TempActor->GetJsonString()));
        }
    }
    else if (DataText)
    {
        DataText->SetText(FText::GetEmpty());
    }
}

void UEntityWindowWidget::CloseWindow()
{
    if (BoundActor)
    {
        BoundActor->OnEntityDataChanged.RemoveDynamic(this, &UEntityWindowWidget::HandleDataChanged);
        BoundActor = nullptr;
    }

    Super::CloseWindow();
}

void UEntityWindowWidget::HandleDataChanged()
{
    if (!BoundActor || !DataText)
        return;

    DataText->SetText(FText::FromString(BoundActor->GetJsonString()));
}