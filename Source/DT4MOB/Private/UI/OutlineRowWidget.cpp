#include "UI/OutlineRowWidget.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBoxSlot.h"
#include "Entities/TempUIActor.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"

bool UOutlineRowWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (RowButton)
    {
        RowButton->OnClicked.AddDynamic(this, &UOutlineRowWidget::HandleRowClicked);
        RowButton->OnHovered.AddDynamic(this, &UOutlineRowWidget::HandleRowHovered);
        RowButton->OnUnhovered.AddDynamic(this, &UOutlineRowWidget::HandleRowUnhovered);
    }

    if (VisibilityButton)
        VisibilityButton->OnClicked.AddDynamic(this, &UOutlineRowWidget::HandleVisibilityClicked);

    return true;
}

void UOutlineRowWidget::SetData(const FString& InThingId, const FString& InTypeKey, const FString& InDisplayName, ATempUIActor* InActor)
{
    ThingId = InThingId;
    TypeKey = InTypeKey;
    BoundActor = InActor;
    bIsVisible = true;
    OnRowVisibilityChanged(bIsVisible);

    const FLinearColor BadgeColor = GetBadgeColor(this, InTypeKey);

    if (TypeLabel)
    {
        TypeLabel->SetText(FText::FromString(GetBadgeLabel(this, InTypeKey)));
        TypeLabel->SetColorAndOpacity(FSlateColor(BadgeColor));
    }

    if (EntityIdLabel)
    {
        EntityIdLabel->SetText(FText::FromString(InThingId));
        EntityIdLabel->SetColorAndOpacity(FSlateColor(bIsHovered ? TextHoverColor : TextNormalColor));
    }

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
}

void UOutlineRowWidget::SetBorderColorPreservingOutline(UBorder* Border, const FLinearColor& Color)
{
    if (!Border)
        return;

    FSlateBrush Brush = Border->Background;

    // A Border widget left un-styled in the Designer defaults to DrawAs=NoDrawType — no amount of
    // TintColor/OutlineSettings.Color change is visible until it's a real drawable brush type.
    if (Brush.DrawAs == ESlateBrushDrawType::NoDrawType)
        Brush.DrawAs = ESlateBrushDrawType::Box;

    Brush.TintColor = FSlateColor(Color);
    Brush.OutlineSettings.Color = FSlateColor(Color);
    Border->SetBrush(Brush);
}

void UOutlineRowWidget::SetEvenRow(bool bEven)
{
    bIsEvenRow = bEven;
    RefreshRowBackground();
}

void UOutlineRowWidget::ApplyTheme_Implementation(UUIThemeData* Theme)
{
    if (!Theme) return;

    EvenRowColor = Theme->RowBackgroundEven;
    OddRowColor = FLinearColor::Transparent;
    HoverColor = Theme->RowHover;
    TextNormalColor = Theme->TextSecondary;
    TextHoverColor = Theme->TextPrimary;

    RefreshRowBackground();
}

void UOutlineRowWidget::RefreshRowBackground()
{
    if (EntityIdLabel)
        EntityIdLabel->SetColorAndOpacity(FSlateColor(bIsHovered ? TextHoverColor : TextNormalColor));

    if (bIsHovered)
    {
        SetBorderColorPreservingOutline(Background, HoverColor);
        return;
    }

    SetBorderColorPreservingOutline(Background, bIsEvenRow ? EvenRowColor : OddRowColor);
}

void UOutlineRowWidget::HandleRowHovered()
{
    bIsHovered = true;
    RefreshRowBackground();
}

void UOutlineRowWidget::HandleRowUnhovered()
{
    bIsHovered = false;
    RefreshRowBackground();
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

void UOutlineRowWidget::OnRowVisibilityChanged_Implementation(bool bVisible)
{
    if (VisibilityIcon)
        VisibilityIcon->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

    if (VisibilityIcon_Hidden)
        VisibilityIcon_Hidden->SetVisibility(bVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
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
