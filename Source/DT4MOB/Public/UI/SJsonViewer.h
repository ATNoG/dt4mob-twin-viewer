/**
 * @file
 * @brief Declares SJsonViewer, the Slate JSON viewer.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FJsonTextMarshaller;
class SMultiLineEditableText;
class SScrollBox;

/**
 * @brief Slate widget that shows read-only, word-wrapped, syntax-coloured JSON in a scroll box.
 * Wrapped for UMG by UJsonViewerWidget.
 * @ingroup ui
 */
class DT4MOB_API SJsonViewer : public SCompoundWidget
{
public:
    /** @brief Slate construction arguments (none). */
    SLATE_BEGIN_ARGS(SJsonViewer) {}
    SLATE_END_ARGS()

    /**
     * @brief Builds the scroll box and the read-only text widget.
     * @param InArgs Slate arguments (none).
     */
    void Construct(const FArguments& InArgs);

    /**
     * @brief Replaces the shown text.
     * @param Json JSON text to show.
     */
    void SetJsonText(const FString& Json);

    /** @brief Scrolls back to the top. */
    void ScrollToStart();

private:
    TSharedPtr<FJsonTextMarshaller> Marshaller;     ///< Colours the text.
    TSharedPtr<SMultiLineEditableText> TextWidget;  ///< Read-only text widget.
    TSharedPtr<SScrollBox> ScrollBox;               ///< Scroll box around the text.
};
