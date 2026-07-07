#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/OutlineRowWidget.h"
#include "OutlinePanelWidget.generated.h"

class ATempUIActor;
class UActorRegistryService;
class UButton;
class UEditableText;
class UScrollBox;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityOpenRequested, ATempUIActor*, Actor);

UCLASS()
class DT4MOB_API UOutlinePanelWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;
    virtual void NativeConstruct() override;

    /** Flips open/closed state and triggers the appropriate slide animation. */
    void TogglePanel();

    UPROPERTY(BlueprintAssignable)
    FOnEntityOpenRequested OnEntityOpenRequested;

    bool IsOpen() const { return bIsOpen; }

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* CloseButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UEditableText* SearchBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UScrollBox* EntityListBox;

    /** Set this in the Blueprint subclass to the WBP_OutlineRow asset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline")
    TSubclassOf<UOutlineRowWidget> RowWidgetClass;

    /** Blueprint plays the slide-in animation and ensures the widget is visible. */
    UFUNCTION(BlueprintImplementableEvent)
    void PlayOpenAnimation();

    /** Blueprint plays the slide-out animation and collapses the widget at the end. */
    UFUNCTION(BlueprintImplementableEvent)
    void PlayCloseAnimation();

private:
    bool bIsOpen = false;

    UPROPERTY()
    UActorRegistryService* Registry = nullptr;

    TArray<UOutlineRowWidget*> AllRows;

    void PopulateAll();
    void AddRow(ATempUIActor* Actor);

    UFUNCTION()
    void HandleCloseClicked();

    UFUNCTION()
    void HandleSearchChanged(const FText& Text);

    UFUNCTION()
    void HandleEntityRegistered(ATempUIActor* Actor);

    UFUNCTION()
    void HandleEntityUnregistered(const FString& ThingId);

    UFUNCTION()
    void HandleRowSelected(const FString& ThingId);
};
