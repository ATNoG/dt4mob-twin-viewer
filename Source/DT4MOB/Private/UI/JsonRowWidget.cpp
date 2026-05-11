#include "UI/JsonRowWidget.h"
#include "UI/JsonTreeNode.h"
#include "UI/JsonViewerWidget.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"

static constexpr float IndentSize = 16.0f;

void UJsonRowWidget::InitRow(UJsonTreeNode* InNode, UJsonViewerWidget* InViewer)
{
    BoundNode   = InNode;
    OwnerViewer = InViewer;

    IndentSpacer->SetWidthOverride(InNode->Depth * IndentSize);

    if (InNode->bIsExpandable)
    {
        FString Arrow = InNode->bIsExpanded ? TEXT("▼ ") : TEXT("▶ ");
        KeyText->SetText(FText::FromString(Arrow + InNode->Key));
    }
    else
    {
        KeyText->SetText(FText::FromString(InNode->Key + TEXT(":")));
    }

    ValueText->SetText(FText::FromString(InNode->Value));
}

FReply UJsonRowWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (BoundNode && BoundNode->bIsExpandable && OwnerViewer
        && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        OwnerViewer->ToggleNode(BoundNode);
        return FReply::Handled();
    }
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
