// EntityWindowWidget.cpp
/** @file EntityWindowWidget.cpp
 *  @brief Implementation of UEntityWindowWidget. All logic documentation is in the header.
 */
#include "UI/EntityWindowWidget.h"
#include "UI/RootHUDWidget.h"
#include "UI/JsonViewerWidget.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Entities/TempUIActor.h"
#include "Services/ActorRegistryService.h"
#include "Kismet/GameplayStatics.h"

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
        if (ATempUIActor *TempActor = Cast<ATempUIActor>(Actor))
            NameText->SetText(FText::FromString(TempActor->GetThingId()));
        else
            NameText->SetText(FText::FromString(Actor->GetName()));
    }

    if (ATempUIActor *TempActor = Cast<ATempUIActor>(Actor))
    {
        BoundActor = TempActor;
        BoundActor->OnEntityDataChanged.AddDynamic(this, &UEntityWindowWidget::HandleDataChanged);

        UE_LOG(LogTemp, Warning, TEXT("EntityWindow: JsonViewer=%s DataText=%s RawJson=%s"),
            JsonViewer ? TEXT("BOUND") : TEXT("NULL"),
            DataText   ? TEXT("BOUND") : TEXT("NULL"),
            TempActor->RawJson.IsValid() ? TEXT("VALID") : TEXT("NULL"));

        if (JsonViewer)
            JsonViewer->SetJsonObject(TempActor->RawJson);
        else if (DataText)
            DataText->SetText(FText::FromString(TempActor->GetJsonString()));

        // Find instrument children and notify Blueprint so it can populate the Instruments tab.
        TArray<ATempUIActor *> Instruments;
        if (UActorRegistryService *Registry = UActorRegistryService::Get(this))
            Instruments = Registry->FindActorsWithPrefix(TempActor->GetThingId() + TEXT(".instrument."));

        UE_LOG(LogTemp, Warning, TEXT("Instrument search prefix: %s | Found: %d"), *(TempActor->GetThingId() + TEXT(".instrument.")), Instruments.Num());

        OnInstrumentsLoaded(Instruments);

        // Overlays tab: only for geo-asset things that are not instruments
        const FString& Id = TempActor->GetThingId();
        const bool bIsGeoAsset = Id.StartsWith(TEXT("geo-asset:")) && !Id.Contains(TEXT(".instrument."));
        if (bIsGeoAsset)
        {
            TArray<AActor*> OverlayActors;
            UGameplayStatics::GetAllActorsWithTag(this, FName("GeoOverlay"), OverlayActors);
            OnOverlaysAvailable(OverlayActors);
        }
        else
        {
            OnOverlaysAvailable({});
        }
    }
    else if (DataText)
    {
        DataText->SetText(FText::GetEmpty());
        OnInstrumentsLoaded({});
        OnOverlaysAvailable({});
    }
}

void UEntityWindowWidget::SetOwnerHUD(URootHUDWidget *HUD)
{
    OwnerHUD = HUD;
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
    if (!BoundActor)
        return;

    if (JsonViewer)
        JsonViewer->SetJsonObject(BoundActor->RawJson);
    else if (DataText)
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