#include "UI/InfoTabWidget.h"
#include "UI/InfoFieldRegistry.h"
#include "Entities/TempUIActor.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

void UInfoTabWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (UInfoFieldRegistry* Reg = GetRegistry())
        Reg->OnInfoFieldsChanged.AddDynamic(this, &UInfoTabWidget::HandleFieldsChanged);
}

void UInfoTabWidget::NativeDestruct()
{
    if (UInfoFieldRegistry* Reg = GetRegistry())
        Reg->OnInfoFieldsChanged.RemoveDynamic(this, &UInfoTabWidget::HandleFieldsChanged);

    SetBoundActor(nullptr);
    Super::NativeDestruct();
}

// ── Public API ────────────────────────────────────────────────────────────────

void UInfoTabWidget::SetBoundActor(ATempUIActor* Actor)
{
    if (IsValid(BoundActor))
        BoundActor->OnEntityDataChanged.RemoveDynamic(this, &UInfoTabWidget::HandleEntityDataChanged);

    BoundActor = Actor;
    CachedTypeKey = FString();

    if (IsValid(BoundActor))
    {
        BoundActor->OnEntityDataChanged.AddDynamic(this, &UInfoTabWidget::HandleEntityDataChanged);

        if (UGameInstance* GI = GetGameInstance())
            if (UDT4MOBEntityFactory* Factory = GI->GetSubsystem<UDT4MOBEntityFactory>())
                CachedTypeKey = Factory->GetTypeKeyForThingId(BoundActor->GetThingId());
    }

    RebuildRows();
}

// ── Row building ──────────────────────────────────────────────────────────────

void UInfoTabWidget::RebuildRows()
{
    if (!PropertyList)
        return;

    PropertyList->ClearChildren();
    LabelBlocks.Reset();
    ValueBlocks.Reset();

    if (!IsValid(BoundActor))
        return;

    UInfoFieldRegistry* Reg = GetRegistry();
    if (!Reg)
        return;

    const TArray<FInfoField> Fields = Reg->GetFields(CachedTypeKey);
    if (Fields.IsEmpty())
        return;

    for (const FInfoField& Field : Fields)
    {
        UHorizontalBox* Row = NewObject<UHorizontalBox>(this);

        // Label
        UTextBlock* Label = NewObject<UTextBlock>(this);
        Label->SetText(FText::FromString(Field.DisplayName));
        Label->SetColorAndOpacity(FSlateColor(LabelColor));
        if (LabelFont.HasValidFont())
            Label->SetFont(LabelFont);

        LabelBlocks.Add(Label);

        UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
        LabelSlot->SetPadding(FMargin(14.f, 7.f, 0.f, 7.f));

        // Value
        UTextBlock* Value = NewObject<UTextBlock>(this);
        Value->SetColorAndOpacity(FSlateColor(ValueColor));
        Value->SetJustification(ETextJustify::Right);
        if (ValueFont.HasValidFont())
            Value->SetFont(ValueFont);

        UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(Value);
        ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ValueSlot->SetVerticalAlignment(VAlign_Center);
        ValueSlot->SetPadding(FMargin(0.f, 7.f, 14.f, 7.f));

        ValueBlocks.Add(Value);

        UVerticalBoxSlot* RowSlot = PropertyList->AddChildToVerticalBox(Row);
        RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
    }

    RefreshValues();
}

void UInfoTabWidget::RefreshValues()
{
    if (!IsValid(BoundActor))
        return;

    UInfoFieldRegistry* Reg = GetRegistry();
    if (!Reg)
        return;

    const TArray<FInfoField> Fields = Reg->GetFields(CachedTypeKey);

    for (int32 i = 0; i < Fields.Num() && i < ValueBlocks.Num(); ++i)
    {
        if (!ValueBlocks[i])
            continue;

        FString Raw = BoundActor->GetRawJsonFieldAny(Fields[i].JsonDotPath);
        const FString Display = Fields[i].Unit.IsEmpty()
            ? Raw
            : FString::Printf(TEXT("%s %s"), *Raw, *Fields[i].Unit);

        ValueBlocks[i]->SetText(FText::FromString(Display));
    }
}

void UInfoTabWidget::RefreshColors()
{
    for (UTextBlock* Block : LabelBlocks)
        if (Block) Block->SetColorAndOpacity(FSlateColor(LabelColor));

    for (UTextBlock* Block : ValueBlocks)
        if (Block) Block->SetColorAndOpacity(FSlateColor(ValueColor));
}

// ── Callbacks ─────────────────────────────────────────────────────────────────

void UInfoTabWidget::HandleEntityDataChanged()
{
    RefreshValues();
}

void UInfoTabWidget::HandleFieldsChanged(const FString& TypeKey)
{
    if (TypeKey == CachedTypeKey)
        RebuildRows();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

UInfoFieldRegistry* UInfoTabWidget::GetRegistry() const
{
    if (APlayerController* PC = GetOwningPlayer())
        return PC->GetLocalPlayer()->GetSubsystem<UInfoFieldRegistry>();
    return nullptr;
}
