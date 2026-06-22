#include "UI/InfoTabWidget.h"
#include "UI/InfoFieldRegistry.h"
#include "Entities/TempUIActor.h"
#include "Entities/DT4MOBEntityFactory.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Styling/CoreStyle.h"
#include "Engine/GameInstance.h"

void UInfoTabWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (UInfoFieldRegistry* Reg = GetRegistry())
        Reg->OnInfoFieldsChanged.AddDynamic(this, &UInfoTabWidget::HandleFieldsChanged);

    if (ConfigureBtn)
        ConfigureBtn->OnClicked.AddDynamic(this, &UInfoTabWidget::HandleConfigureClicked);
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

    const FSlateFontInfo ResolvedLabelFont = LabelFont.HasValidFont()
        ? LabelFont : FCoreStyle::GetDefaultFontStyle("Regular", 16);
    const FSlateFontInfo ResolvedValueFont = ValueFont.HasValidFont()
        ? ValueFont : FCoreStyle::GetDefaultFontStyle("Bold", 16);

    for (int32 i = 0; i < Fields.Num(); i++)
    {
        const FInfoField& Field = Fields[i];

        UBorder* RowBg = WidgetTree->ConstructWidget<UBorder>();
        RowBg->SetBrushColor(i % 2 != 0 ? FLinearColor(1.f, 1.f, 1.f, 0.06f) : FLinearColor(0.f, 0.f, 0.f, 0.f));
        RowBg->SetPadding(FMargin(0.f));
        RowBg->SetHorizontalAlignment(HAlign_Fill);
        RowBg->SetVerticalAlignment(VAlign_Fill);
        UVerticalBoxSlot* RowBgSlot = PropertyList->AddChildToVerticalBox(RowBg);
        RowBgSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        RowBgSlot->SetHorizontalAlignment(HAlign_Fill);

        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        RowBg->SetContent(Row);

        UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>();
        Label->SetText(FText::FromString(Field.DisplayName));
        Label->SetColorAndOpacity(FSlateColor(LabelColor));
        Label->SetFont(ResolvedLabelFont);
        LabelBlocks.Add(Label);

        UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
        LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        LabelSlot->SetVerticalAlignment(VAlign_Center);
        LabelSlot->SetPadding(FMargin(14.f, 8.f, 0.f, 8.f));

        UTextBlock* Value = WidgetTree->ConstructWidget<UTextBlock>();
        Value->SetColorAndOpacity(FSlateColor(ValueColor));
        Value->SetJustification(ETextJustify::Right);
        Value->SetFont(ResolvedValueFont);
        ValueBlocks.Add(Value);

        UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(Value);
        ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        ValueSlot->SetVerticalAlignment(VAlign_Center);
        ValueSlot->SetPadding(FMargin(0.f, 8.f, 14.f, 8.f));
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

void UInfoTabWidget::HandleConfigureClicked()
{
    OnConfigureRequested.Broadcast();
}

// ── Helpers ───────────────────────────────────────────────────────────────────

UInfoFieldRegistry* UInfoTabWidget::GetRegistry() const
{
    if (APlayerController* PC = GetOwningPlayer())
        return PC->GetLocalPlayer()->GetSubsystem<UInfoFieldRegistry>();
    return nullptr;
}
