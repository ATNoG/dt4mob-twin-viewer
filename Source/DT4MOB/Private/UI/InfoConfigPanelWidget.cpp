#include "UI/InfoConfigPanelWidget.h"
#include "UI/InfoFieldRegistry.h"
#include "UI/JsonFlattener.h"
#include "Entities/TempUIActor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"

static FSlateBrush MakeOverlayBrush(const FLinearColor& Color)
{
    FSlateBrush Brush;
    Brush.DrawAs = Color.A > 0.f ? ESlateBrushDrawType::Box : ESlateBrushDrawType::NoDrawType;
    Brush.TintColor = FSlateColor(Color);
    return Brush;
}

void UInfoConfigPanelWidget::ApplyTheme_Implementation(UUIThemeData* Theme)
{
    if (!Theme) return;

    auto MakeTransparentBtn = [](UButton* Btn)
    {
        if (!Btn) return;
        FButtonStyle Style = Btn->GetStyle();
        Style.Normal  = MakeOverlayBrush(FLinearColor::Transparent);
        Style.Hovered = MakeOverlayBrush(FLinearColor::Transparent);
        Style.Pressed = MakeOverlayBrush(FLinearColor::Transparent);
        Btn->SetStyle(Style);
    };

    MakeTransparentBtn(SaveBtn);
    MakeTransparentBtn(ResetBtn);

    SaveBorderNormal  = Theme->Accent;
    SaveBorderHovered = FLinearColor(Theme->Accent.R * 0.85f, Theme->Accent.G * 0.85f, Theme->Accent.B * 0.85f, 1.f);
    SaveBorderPressed = FLinearColor(Theme->Accent.R * 0.70f, Theme->Accent.G * 0.70f, Theme->Accent.B * 0.70f, 1.f);

    if (SaveBtnBorder) SaveBtnBorder->SetBrushColor(SaveBorderNormal);

    ResetBorderNormal  = Theme->BackgroundSecondary;
    ResetBorderHovered = Theme->Hover;
    ResetBorderPressed = Theme->Pressed;

    if (ResetBtnBorder) ResetBtnBorder->SetBrushColor(ResetBorderNormal);
}

bool UInfoConfigPanelWidget::Initialize()
{
    if (!Super::Initialize())
        return false;

    if (SaveBtn)
    {
        SaveBtn->OnClicked.AddDynamic(this, &UInfoConfigPanelWidget::HandleSave);
        SaveBtn->OnHovered.AddDynamic(this, &UInfoConfigPanelWidget::HandleSaveBtnHovered);
        SaveBtn->OnUnhovered.AddDynamic(this, &UInfoConfigPanelWidget::HandleSaveBtnUnhovered);
        SaveBtn->OnPressed.AddDynamic(this, &UInfoConfigPanelWidget::HandleSaveBtnPressed);
        SaveBtn->OnReleased.AddDynamic(this, &UInfoConfigPanelWidget::HandleSaveBtnReleased);
    }
    if (ResetBtn)
    {
        ResetBtn->OnClicked.AddDynamic(this, &UInfoConfigPanelWidget::HandleReset);
        ResetBtn->OnHovered.AddDynamic(this, &UInfoConfigPanelWidget::HandleResetBtnHovered);
        ResetBtn->OnUnhovered.AddDynamic(this, &UInfoConfigPanelWidget::HandleResetBtnUnhovered);
        ResetBtn->OnPressed.AddDynamic(this, &UInfoConfigPanelWidget::HandleResetBtnPressed);
        ResetBtn->OnReleased.AddDynamic(this, &UInfoConfigPanelWidget::HandleResetBtnReleased);
    }
    if (SelectAllCB) SelectAllCB->OnCheckStateChanged.AddDynamic(this, &UInfoConfigPanelWidget::HandleSelectAllChanged);
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
        CB->OnCheckStateChanged.AddDynamic(this, &UInfoConfigPanelWidget::HandleRowCheckChanged);
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

    UpdateSelectAllState();
}

void UInfoConfigPanelWidget::HandleSaveBtnHovered()   { if (SaveBtnBorder)  SaveBtnBorder->SetBrushColor(SaveBorderHovered); }
void UInfoConfigPanelWidget::HandleSaveBtnUnhovered() { if (SaveBtnBorder)  SaveBtnBorder->SetBrushColor(SaveBorderNormal); }
void UInfoConfigPanelWidget::HandleSaveBtnPressed()   { if (SaveBtnBorder)  SaveBtnBorder->SetBrushColor(SaveBorderPressed); }
void UInfoConfigPanelWidget::HandleSaveBtnReleased()  { if (SaveBtnBorder)  SaveBtnBorder->SetBrushColor(SaveBorderHovered); }

void UInfoConfigPanelWidget::HandleResetBtnHovered()   { if (ResetBtnBorder) ResetBtnBorder->SetBrushColor(ResetBorderHovered); }
void UInfoConfigPanelWidget::HandleResetBtnUnhovered() { if (ResetBtnBorder) ResetBtnBorder->SetBrushColor(ResetBorderNormal); }
void UInfoConfigPanelWidget::HandleResetBtnPressed()   { if (ResetBtnBorder) ResetBtnBorder->SetBrushColor(ResetBorderPressed); }
void UInfoConfigPanelWidget::HandleResetBtnReleased()  { if (ResetBtnBorder) ResetBtnBorder->SetBrushColor(ResetBorderHovered); }

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

void UInfoConfigPanelWidget::HandleSelectAllChanged(bool bIsChecked)
{
    bUpdatingSelectAll = true;
    for (UCheckBox* CB : CheckBoxes)
        if (CB) CB->SetIsChecked(bIsChecked);
    bUpdatingSelectAll = false;

    // Force a definite state — no undetermined after a deliberate click
    if (SelectAllCB)
        SelectAllCB->SetCheckedState(bIsChecked ? ECheckBoxState::Checked : ECheckBoxState::Unchecked);
}

void UInfoConfigPanelWidget::HandleRowCheckChanged(bool /*bIsChecked*/)
{
    if (!bUpdatingSelectAll)
        UpdateSelectAllState();
}

void UInfoConfigPanelWidget::UpdateSelectAllState()
{
    if (!SelectAllCB) return;

    int32 CheckedCount = 0;
    for (UCheckBox* CB : CheckBoxes)
        if (CB && CB->IsChecked()) CheckedCount++;

    if (CheckedCount == 0)
        SelectAllCB->SetCheckedState(ECheckBoxState::Unchecked);
    else if (CheckedCount == CheckBoxes.Num())
        SelectAllCB->SetCheckedState(ECheckBoxState::Checked);
    else
        SelectAllCB->SetCheckedState(ECheckBoxState::Undetermined);
}
