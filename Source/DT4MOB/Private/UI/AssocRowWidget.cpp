#include "UI/AssocRowWidget.h"
#include "UI/OutlineRowWidget.h"
#include "Entities/TempUIActor.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Engine/GameInstance.h"

bool UAssocRowWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (OpenButton)
    {
        OpenButton->OnClicked.AddDynamic(this, &UAssocRowWidget::HandleOpenClicked);
        OpenButton->SetStyle(UOutlineRowWidget::MakePillButtonStyle());
    }

    return true;
}

void UAssocRowWidget::SetActor(ATempUIActor* Actor)
{
    if (!IsValid(Actor))
        return;

    CachedThingId = Actor->GetThingId();

    if (ThingIdLabel)
        ThingIdLabel->SetText(FText::FromString(CachedThingId));

    FString TypeKey;
    if (UGameInstance* GI = GetGameInstance())
        if (UDT4MOBEntityFactory* Factory = GI->GetSubsystem<UDT4MOBEntityFactory>())
            TypeKey = Factory->GetTypeKeyForThingId(CachedThingId);

    const FString BadgeLabel = UOutlineRowWidget::GetBadgeLabel(TypeKey);
    const FLinearColor BadgeColor = UOutlineRowWidget::GetBadgeColor(TypeKey);

    if (TypeLabel)
    {
        TypeLabel->SetText(FText::FromString(BadgeLabel));
        TypeLabel->SetColorAndOpacity(FSlateColor(BadgeColor));
    }

    if (TypeBadge)
    {
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
        Brush.TintColor = FSlateColor(BadgeColor.CopyWithNewOpacity(0.15f));
        Brush.OutlineSettings.Width = 1.f;
        Brush.OutlineSettings.Color = FSlateColor(BadgeColor.CopyWithNewOpacity(0.4f));
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        Brush.OutlineSettings.CornerRadii = FVector4(0.f, 0.f, 0.f, 0.f);
        TypeBadge->SetBrush(Brush);
    }
}

void UAssocRowWidget::SetEvenRow(bool bEven)
{
    if (RowBackground)
    {
        RowBackground->SetBrushColor(bEven ? FLinearColor(0.f, 0.f, 0.f, 0.f)
                                           : FLinearColor(1.f, 1.f, 1.f, 0.06f));
        RowBackground->SetPadding(FMargin(14.f, 8.f));
    }
}

void UAssocRowWidget::HandleOpenClicked()
{
    OnOpenRequested.Broadcast(CachedThingId);
}
