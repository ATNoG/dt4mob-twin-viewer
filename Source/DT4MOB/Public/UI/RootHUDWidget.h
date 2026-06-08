#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/ToolbarWidget.h"
#include "UI/OutlinePanelWidget.h"
#include "UI/EntityWindowWidget.h"
#include "RootHUDWidget.generated.h"

class UUIManager;
class UCanvasPanel;

UCLASS()
class DT4MOB_API URootHUDWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UToolbarWidget* Toolbar;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UOutlinePanelWidget* OutlinePanel;

    /** Full-screen canvas that entity windows are spawned into. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UCanvasPanel* EntityWindowContainer;

    /** Blueprint class to instantiate for each entity window. Set this in WBP_RootHud defaults. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EntityWindow")
    TSubclassOf<UEntityWindowWidget> EntityWindowClass;

    UPROPERTY()
    UUIManager* UIManager = nullptr;

private:
    /** ThingId → open window. Weak pointers so closed (RemoveFromParent) windows auto-expire. */
    TMap<FString, TWeakObjectPtr<UEntityWindowWidget>> OpenWindows;

    void SpawnOrFocusWindow(ATempUIActor* Actor);

    UFUNCTION()
    void HandleOutlineToggled();

    UFUNCTION()
    void HandleEntityTypeFilterChanged(const FString& TypeKey);

    UFUNCTION()
    void HandleEntitySelected(AActor* Actor);

    UFUNCTION()
    void HandleEntityWindowClosed(const FString& ThingId);
};
