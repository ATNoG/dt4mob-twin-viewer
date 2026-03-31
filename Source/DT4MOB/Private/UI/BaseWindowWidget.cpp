// BaseWindowWidget.cpp
/** @file BaseWindowWidget.cpp
 *  @brief Implementation of UBaseWindowWidget. All logic documentation is in the header.
 */
#include "UI/BaseWindowWidget.h"

void UBaseWindowWidget::OpenWindow()
{
    SetVisibility(ESlateVisibility::Visible);
}

void UBaseWindowWidget::CloseWindow()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

void UBaseWindowWidget::OnBindData_Implementation(AActor *Actor)
{
    // Base implementation - override in derived classes
}