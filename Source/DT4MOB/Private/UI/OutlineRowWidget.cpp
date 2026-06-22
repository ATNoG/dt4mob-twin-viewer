#include "UI/OutlineRowWidget.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Entities/TempUIActor.h"

void UOutlineRowWidget::SetData(const FString& InThingId, const FString& InTypeKey, const FString& InDisplayName, ATempUIActor* InActor)
{
    ThingId = InThingId;
    TypeKey = InTypeKey;
    BoundActor = InActor;
    bIsVisible = true;

    const FLinearColor BadgeColor = GetBadgeColor(InTypeKey);

    if (TypeLabel)
    {
        TypeLabel->SetText(FText::FromString(GetBadgeLabel(InTypeKey)));
        TypeLabel->SetColorAndOpacity(FSlateColor(BadgeColor));
    }

    if (EntityIdLabel)
        EntityIdLabel->SetText(FText::FromString(InThingId));

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

FLinearColor UOutlineRowWidget::GetBadgeColor(const FString& Key)
{
    if (Key == TEXT("traci"))        return FLinearColor(0.557f, 0.714f, 1.000f); // blue   — vehicles
    if (Key == TEXT("meteo"))        return FLinearColor(0.800f, 0.533f, 1.000f); // purple — meteo
    if (Key == TEXT("fire:"))        return FLinearColor(1.000f, 0.380f, 0.180f); // orange — fire
    if (Key == TEXT("sign"))         return FLinearColor(0.859f, 0.659f, 0.337f); // amber  — signs
    if (Key == TEXT("barrier"))      return FLinearColor(0.859f, 0.659f, 0.337f); // amber  — barriers
    if (Key == TEXT("muro-talude"))  return FLinearColor(0.557f, 0.882f, 0.533f); // green  — slopes
    if (Key == TEXT(".instrument.")) return FLinearColor(0.557f, 0.882f, 0.533f); // green  — instruments
    if (Key == TEXT("geo-asset"))    return FLinearColor(0.557f, 0.882f, 0.533f); // green  — geo assets
    if (Key.StartsWith(TEXT("tolls")))   return FLinearColor(0.557f, 0.882f, 0.533f); // green  — toll
    if (Key.StartsWith(TEXT("equivia"))) return FLinearColor(0.859f, 0.659f, 0.337f); // amber  — road infra
    return FLinearColor(0.475f, 0.475f, 0.475f);                                       // gray   — default
}

FString UOutlineRowWidget::GetBadgeLabel(const FString& Key)
{
    if (Key == TEXT("traci"))        return TEXT("CAR");
    if (Key == TEXT("meteo"))        return TEXT("METEO");
    if (Key == TEXT("fire:"))        return TEXT("FIRE");
    if (Key == TEXT("sign"))         return TEXT("SIGN");
    if (Key == TEXT("barrier"))      return TEXT("BARRIER");
    if (Key == TEXT("muro-talude"))  return TEXT("SLOPE");
    if (Key == TEXT(".instrument.")) return TEXT("INSTR");
    if (Key == TEXT("geo-asset"))    return TEXT("GEO");
    if (Key.StartsWith(TEXT("tolls:camera"))) return TEXT("CAM");
    if (Key.StartsWith(TEXT("tolls")))        return TEXT("TOLL");
    if (Key.StartsWith(TEXT("equivia")))      return TEXT("ROAD");
    return Key.Left(6).ToUpper();
}
