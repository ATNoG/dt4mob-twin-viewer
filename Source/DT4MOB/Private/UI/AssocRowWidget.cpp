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
        OpenButton->OnClicked.AddDynamic(this, &UAssocRowWidget::HandleOpenClicked);

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
        FLinearColor BgColor = BadgeColor;
        BgColor.A = 0.18f;
        TypeBadge->SetBrushColor(BgColor);
    }
}

void UAssocRowWidget::HandleOpenClicked()
{
    OnOpenRequested.Broadcast(CachedThingId);
}
