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
class UBorder;
class UWidgetAnimation;
class UTextBlock;

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
    /** Panel header ("Entities"). Must be named "HeaderLabel" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* HeaderLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* CloseButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UEditableText* SearchBox;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UScrollBox* EntityListBox;

    /** Must be named "HeaderBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* HeaderBorder;

    /** Must be named "SearchBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* SearchBorder;

    /** Must be named "ListBorder" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* ListBorder;

    /** "All Entities" list heading. Must be named "OverviewText" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* OverviewText;

    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

    /** Set this in the Blueprint subclass to the WBP_OutlineRow asset. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Outline")
    TSubclassOf<UOutlineRowWidget> RowWidgetClass;

    /** Slide-in animation. Must be named "OpenAnimation" in the Blueprint's Animations panel. */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    UWidgetAnimation* OpenAnimation;

    /** Slide-out animation. Must be named "CloseAnimation" in the Blueprint's Animations panel. */
    UPROPERTY(Transient, meta = (BindWidgetAnimOptional))
    UWidgetAnimation* CloseAnimation;

private:
    bool bIsOpen = false;

    UPROPERTY()
    UActorRegistryService* Registry = nullptr;

    TArray<UOutlineRowWidget*> AllRows;

    void PopulateAll();
    void AddRow(ATempUIActor* Actor);
    void PlayOpenAnimation();
    void PlayCloseAnimation();

    UFUNCTION()
    void HandleCloseAnimationFinished();

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
