#include "UI/OutlineRowWidget.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SizeBoxSlot.h"
#include "Entities/TempUIActor.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"

void UOutlineRowWidget::SetData(const FString& InThingId, const FString& InTypeKey, const FString& InDisplayName, ATempUIActor* InActor)
{
    ThingId = InThingId;
    TypeKey = InTypeKey;
    BoundActor = InActor;
    bIsVisible = true;

    const FLinearColor BadgeColor = GetBadgeColor(this, InTypeKey);

    if (TypeLabel)
    {
        TypeLabel->SetText(FText::FromString(GetBadgeLabel(this, InTypeKey)));
        TypeLabel->SetColorAndOpacity(FSlateColor(BadgeColor));
    }

    if (EntityIdLabel)
        EntityIdLabel->SetText(FText::FromString(InThingId));

    if (TypeBadge)
    {
        if (USizeBoxSlot* BadgeSlot = Cast<USizeBoxSlot>(TypeBadge->Slot))
        {
            BadgeSlot->SetHorizontalAlignment(HAlign_Fill);
            BadgeSlot->SetVerticalAlignment(VAlign_Fill);
        }

        FSlateBrush Brush;
        Brush.ImageSize = FVector2D::ZeroVector;
        Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
        Brush.TintColor = FSlateColor(BadgeColor.CopyWithNewOpacity(0.15f));
        Brush.OutlineSettings.Width = 1.f;
        Brush.OutlineSettings.Color = FSlateColor(BadgeColor.CopyWithNewOpacity(0.4f));
        Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
        Brush.OutlineSettings.CornerRadii = FVector4(0.f, 0.f, 0.f, 0.f);
        TypeBadge->SetBrush(Brush);
    }

    if (RowButton)
        RowButton->OnClicked.AddDynamic(this, &UOutlineRowWidget::HandleRowClicked);

    if (VisibilityButton)
        VisibilityButton->OnClicked.AddDynamic(this, &UOutlineRowWidget::HandleVisibilityClicked);
}

void UOutlineRowWidget::SetEvenRow(bool bEven)
{
    if (RowBackground)
        RowBackground->SetBrushColor(bEven ? FLinearColor(0.f, 0.f, 0.f, 0.f)
                                           : FLinearColor(1.f, 1.f, 1.f, 0.06f));
}

void UOutlineRowWidget::HandleRowClicked()
{
    OnRowSelected.Broadcast(ThingId);
}

void UOutlineRowWidget::HandleVisibilityClicked()
{
    if (!BoundActor.IsValid())
        return;

    bIsVisible = !bIsVisible;
    BoundActor->SetActorHiddenInGame(!bIsVisible);
    OnRowVisibilityChanged(bIsVisible);
}

FLinearColor UOutlineRowWidget::GetBadgeColor(const UObject* WorldContextObject, const FString& Key)
{
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject))
        if (UDT4MOBEntityFactory* Factory = GI->GetSubsystem<UDT4MOBEntityFactory>())
            return Factory->GetExtensionForType(Key)->GetBadgeColor();

    return FLinearColor(0.475f, 0.475f, 0.475f); // gray — no factory available
}

FButtonStyle UOutlineRowWidget::MakePillButtonStyle()
{
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
    return Style;
}

FString UOutlineRowWidget::GetBadgeLabel(const UObject* WorldContextObject, const FString& Key)
{
    if (UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject))
        if (UDT4MOBEntityFactory* Factory = GI->GetSubsystem<UDT4MOBEntityFactory>())
            return Factory->GetExtensionForType(Key)->GetBadgeLabel(Key);

    return Key.Left(6).ToUpper(); // no factory available
}
