/**
 * @file
 * @brief Declares UJsonViewerWidget, the UMG JSON viewer.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "JsonViewerWidget.generated.h"

class SJsonViewer;

/**
 * @brief UMG wrapper around SJsonViewer so the JSON viewer can be placed in widget Blueprints.
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UJsonViewerWidget : public UWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Replaces the shown text.
     * @param Json JSON text to show.
     */
    UFUNCTION(BlueprintCallable, Category = "JsonViewer")
    void SetJsonText(const FString& Json);

    /** @brief Scrolls back to the top. */
    UFUNCTION(BlueprintCallable, Category = "JsonViewer")
    void ScrollToStart();

protected:
    /**
     * @brief Creates the underlying SJsonViewer.
     * @return The Slate widget.
     */
    virtual TSharedRef<SWidget> RebuildWidget() override;

    /**
     * @brief Releases the Slate widget.
     * @param bReleaseChildren Passed to the base class.
     */
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
    /** @brief The Slate widget, valid between RebuildWidget() and ReleaseSlateResources(). */
    TSharedPtr<SJsonViewer> MyViewer;
};
