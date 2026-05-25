#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ToolbarWidget.h"
#include "RootHUDWidget.generated.h"

class UUIManager;

/**
 * @brief Top-level HUD widget — added to the viewport once and owns all UI panels.
 *
 * Acts purely as a coordinator: it holds references to child panels and routes
 * events between them.  Child panels are added incrementally as they are built;
 * this class imposes no required Blueprint bindings until a panel is ready.
 */
UCLASS()
class DT4MOB_API URootHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

protected:
    /** Must be named "Toolbar" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UToolbarWidget* Toolbar;

    /** Cached LocalPlayer UIManager — used by child panels to read/write shared UI state. */
    UPROPERTY()
    UUIManager* UIManager = nullptr;

private:
    UFUNCTION()
    void HandleOutlineToggled();

    UFUNCTION()
    void HandleEntityTypeFilterChanged(const FString& TypeKey);
};
