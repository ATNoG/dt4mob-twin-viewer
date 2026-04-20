// EntityWindowWidget.cpp
/** @file EntityWindowWidget.cpp
 *  @brief Implementation of UEntityWindowWidget. All logic documentation is in the header.
 */
#include "UI/EntityWindowWidget.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
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

void UEntityWindowWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!DataText || !NameText || !SizeBox) return;

    float AvailableHeight = SizeBox->GetHeightOverride();
    
    // Map height range (e.g. 300 to 800) to a 0-6 bonus
    float Bonus = FMath::Clamp((AvailableHeight - 200.f) / (1500.f - 200.f), 0.f, 1.f) * 6.f;
    int32 BonusInt = FMath::RoundToInt(Bonus);

    FSlateFontInfo DataFont = DataText->GetFont();
    int32 NewDataSize = 14 + BonusInt;
    if (DataFont.Size != NewDataSize)
    {
        DataFont.Size = NewDataSize;
        DataText->SetFont(DataFont);
    }

    FSlateFontInfo NameFont = NameText->GetFont();
    int32 NewNameSize = 18 + BonusInt;
    if (NameFont.Size != NewNameSize)
    {
        NameFont.Size = NewNameSize;
        NameText->SetFont(NameFont);
    }
}