#include "UI/InfoConfigPanelWidget.h"
#include "UI/InfoFieldRegistry.h"
#include "UI/JsonFlattener.h"
#include "Entities/TempUIActor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

bool UInfoConfigPanelWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (SaveBtn)       SaveBtn->OnClicked.AddDynamic(this, &UInfoConfigPanelWidget::HandleSave);
    if (ResetBtn)      ResetBtn->OnClicked.AddDynamic(this, &UInfoConfigPanelWidget::HandleReset);
    if (CloseBtn)      CloseBtn->OnClicked.AddDynamic(this, &UInfoConfigPanelWidget::HandleClose);
    if (SelectAllBtn)  SelectAllBtn->OnClicked.AddDynamic(this, &UInfoConfigPanelWidget::HandleSelectAll);
    if (DeselectAllBtn) DeselectAllBtn->OnClicked.AddDynamic(this, &UInfoConfigPanelWidget::HandleDeselectAll);

    return true;
}

void UInfoConfigPanelWidget::Setup(ATempUIActor* Actor, const FString& TypeKey, UInfoFieldRegistry* Registry)
{
    CachedTypeKey  = TypeKey;
    CachedRegistry = Registry;
    Candidates     = FJsonFlattener::Flatten(Actor->GetRawJsonObject());

    // Merge in any default schema fields missing from this entity's JSON
    if (Registry)
    {
        TSet<FString> PresentPaths;
        for (const FInfoFieldCandidate& C : Candidates)
            PresentPaths.Add(C.DotPath);

        for (const FInfoField& Default : Registry->GetDefaultFields(TypeKey))
        {
            if (!PresentPaths.Contains(Default.JsonDotPath))
            {
                FInfoFieldCandidate Missing;
                Missing.DotPath        = Default.JsonDotPath;
                Missing.SuggestedLabel = Default.DisplayName;
                Missing.CurrentValue   = TEXT("—");
                Missing.bMissing       = true;
                Candidates.Add(Missing);
            }
        }
    }

    BuildRows();
}

void UInfoConfigPanelWidget::BuildRows()
{
    if (!CandidateList)
        return;

    CandidateList->ClearChildren();
    CheckBoxes.Reset();

    const TArray<FInfoField> ActiveFields = CachedRegistry ? CachedRegistry->GetFields(CachedTypeKey) : TArray<FInfoField>();

    // Build a set of active dot-paths for quick lookup
    TSet<FString> ActivePaths;
    for (const FInfoField& F : ActiveFields)
        ActivePaths.Add(F.JsonDotPath);

    const FSlateFontInfo PathFont  = FCoreStyle::GetDefaultFontStyle("Regular", 10);
    const FSlateFontInfo ValueFont = FCoreStyle::GetDefaultFontStyle("Regular", 10);

    for (const FInfoFieldCandidate& Candidate : Candidates)
    {
        UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();

        // Checkbox
        UCheckBox* CB = WidgetTree->ConstructWidget<UCheckBox>();
        CB->SetIsChecked(ActivePaths.Contains(Candidate.DotPath));
        CheckBoxes.Add(CB);

        UHorizontalBoxSlot* CBSlot = Row->AddChildToHorizontalBox(CB);
        CBSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        CBSlot->SetVerticalAlignment(VAlign_Center);
        CBSlot->SetPadding(FMargin(8.f, 0.f, 6.f, 0.f));

        // Dot-path label — dimmed when the field is absent from this entity
        const FLinearColor PathColor = Candidate.bMissing
            ? FLinearColor(0.4f, 0.4f, 0.4f, 1.f)
            : FLinearColor(0.7f, 0.7f, 0.7f, 1.f);

        UTextBlock* PathLabel = WidgetTree->ConstructWidget<UTextBlock>();
        PathLabel->SetText(FText::FromString(Candidate.DotPath));
        PathLabel->SetColorAndOpacity(FSlateColor(PathColor));
        PathLabel->SetFont(PathFont);
        PathLabel->SetClipping(EWidgetClipping::ClipToBounds);

        UHorizontalBoxSlot* PathSlot = Row->AddChildToHorizontalBox(PathLabel);
        PathSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
        PathSlot->SetVerticalAlignment(VAlign_Center);
        PathSlot->SetPadding(FMargin(0.f, 6.f, 8.f, 6.f));

        // Current value — "—" in a distinct color when missing from this entity
        const FLinearColor ValueColor = Candidate.bMissing
            ? FLinearColor(0.3f, 0.3f, 0.3f, 1.f)
            : FLinearColor(0.5f, 0.5f, 0.5f, 1.f);

        UTextBlock* ValueLabel = WidgetTree->ConstructWidget<UTextBlock>();
        ValueLabel->SetText(FText::FromString(Candidate.CurrentValue));
        ValueLabel->SetColorAndOpacity(FSlateColor(ValueColor));
        ValueLabel->SetFont(ValueFont);
        ValueLabel->SetJustification(ETextJustify::Right);

        UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(ValueLabel);
        ValueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
        ValueSlot->SetVerticalAlignment(VAlign_Center);
        ValueSlot->SetPadding(FMargin(0.f, 6.f, 8.f, 6.f));

        UScrollBoxSlot* RowSlot = Cast<UScrollBoxSlot>(CandidateList->AddChild(Row));
        if (RowSlot)
        {
            RowSlot->SetPadding(FMargin(0.f));
            RowSlot->SetHorizontalAlignment(HAlign_Fill);
        }
    }
}

void UInfoConfigPanelWidget::HandleSave()
{
    if (!CachedRegistry)
        return;

    TArray<FInfoField> NewFields;
    for (int32 i = 0; i < Candidates.Num() && i < CheckBoxes.Num(); i++)
    {
        if (CheckBoxes[i] && CheckBoxes[i]->IsChecked())
            NewFields.Add(FInfoField(Candidates[i].SuggestedLabel, Candidates[i].DotPath));
    }

    CachedRegistry->SetFields(CachedTypeKey, NewFields);
    OnClosed.Broadcast();
}

void UInfoConfigPanelWidget::HandleReset()
{
    if (CachedRegistry)
        CachedRegistry->ResetToDefaults(CachedTypeKey);
    OnClosed.Broadcast();
}

void UInfoConfigPanelWidget::HandleClose()
{
    OnClosed.Broadcast();
}

void UInfoConfigPanelWidget::HandleSelectAll()
{
    for (UCheckBox* CB : CheckBoxes)
        if (CB) CB->SetIsChecked(true);
}

void UInfoConfigPanelWidget::HandleDeselectAll()
{
    for (UCheckBox* CB : CheckBoxes)
        if (CB) CB->SetIsChecked(false);
}
