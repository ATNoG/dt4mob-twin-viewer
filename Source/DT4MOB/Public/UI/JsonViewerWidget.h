#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "JsonViewerWidget.generated.h"

class SJsonViewer;

UCLASS()
class DT4MOB_API UJsonViewerWidget : public UWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "JsonViewer")
    void SetJsonText(const FString& Json);

    UFUNCTION(BlueprintCallable, Category = "JsonViewer")
    void ScrollToStart();

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    TSharedPtr<SJsonViewer> MyViewer;
};
