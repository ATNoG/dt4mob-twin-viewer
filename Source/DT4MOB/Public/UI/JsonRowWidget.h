#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "JsonRowWidget.generated.h"

class UTextBlock;
class USizeBox;
class UJsonTreeNode;
class UJsonViewerWidget;

UCLASS()
class DT4MOB_API UJsonRowWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(meta=(BindWidget)) UTextBlock* KeyText;
    UPROPERTY(meta=(BindWidget)) UTextBlock* ValueText;
    UPROPERTY(meta=(BindWidget)) USizeBox*   IndentSpacer;

    void InitRow(UJsonTreeNode* InNode, UJsonViewerWidget* InViewer);

protected:
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
    UPROPERTY() UJsonTreeNode*     BoundNode   = nullptr;
    UPROPERTY() UJsonViewerWidget* OwnerViewer = nullptr;
};
