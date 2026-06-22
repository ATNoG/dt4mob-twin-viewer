#include "UI/AssocRowWidget.h"
#include "UI/OutlineRowWidget.h"
#include "Entities/TempUIActor.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Engine/GameInstance.h"
#include "Styling/CoreStyle.h"

bool UAssocRowWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (OpenButton)
    {
        OpenButton->OnClicked.AddDynamic(this, &UAssocRowWidget::HandleOpenClicked);

        FSlateBrush NormalBrush;
        NormalBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
        NormalBrush.TintColor = FSlateColor(FLinearColor::FromSRGBColor(FColor(0x23, 0x23, 0x23, 0xFF)));
        NormalBrush.OutlineSettings.Width = 1.f;
        NormalBrush.OutlineSettings.Color = FSlateColor(FLinearColor::FromSRGBColor(FColor(0x12, 0x12, 0x12, 0xFF)));
        NormalBrush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        NormalBrush.OutlineSettings.CornerRadii = FVector4(0.f, 0.f, 0.f, 0.f);

        FSlateBrush HoveredBrush = NormalBrush;
        HoveredBrush.TintColor = FSlateColor(FLinearColor::FromSRGBColor(FColor(0x2e, 0x2e, 0x2e, 0xFF)));

        FSlateBrush PressedBrush = NormalBrush;
        PressedBrush.TintColor = FSlateColor(FLinearColor::FromSRGBColor(FColor(0x1a, 0x1a, 0x1a, 0xFF)));

        FButtonStyle Style = FCoreStyle::Get().GetWidgetStyle<FButtonStyle>("Button");
        Style.SetNormal(NormalBrush);
        Style.SetHovered(HoveredBrush);
        Style.SetPressed(PressedBrush);
        Style.NormalPadding = FMargin(8.f, 4.f);
        Style.PressedPadding = FMargin(8.f, 5.f, 8.f, 3.f);
        OpenButton->SetStyle(Style);
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
