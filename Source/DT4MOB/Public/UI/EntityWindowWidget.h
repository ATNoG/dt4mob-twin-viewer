#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "EntityWindowWidget.generated.h"

class ATempUIActor;
class UTextBlock;
class UButton;
class UBorder;
class UWidgetSwitcher;
class UJsonTabWidget;
class UInfoTabWidget;
class UAssocTabWidget;
class UModelsTabWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEntityWindowClosed, const FString&, ThingId);

UCLASS()
class DT4MOB_API UEntityWindowWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;
    virtual void NativeDestruct() override;

    /** Binds this window to an actor. Call this after adding the widget to the viewport. */
    void OpenForActor(ATempUIActor* Actor);

    /** Broadcast when the close button is pressed, before RemoveFromParent. */
    UPROPERTY(BlueprintAssignable)
    FOnEntityWindowClosed OnClosed;

protected:
    // ── Header ────────────────────────────────────────────────────────────────

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* EntityIdTitle;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* CloseButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* TypeBadge;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* TypeLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* StatusLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* ThingIdSubLabel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* GrafanaButton;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EntityWindow")
    FString GrafanaUrlJsonPath = TEXT("attributes.grafana_url");

    // ── Tab bar ───────────────────────────────────────────────────────────────

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* TabInfoBtn;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* TabJsonBtn;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* TabAssocBtn;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* TabModelsBtn;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UWidgetSwitcher* TabSwitcher;

    // ── Tab content ───────────────────────────────────────────────────────────

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UInfoTabWidget* InfoTabWidget;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UJsonTabWidget* JsonTabWidget;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UAssocTabWidget* AssocTabWidget;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UModelsTabWidget* ModelsTabWidget;

    // ── Blueprint hooks ───────────────────────────────────────────────────────

    UFUNCTION(BlueprintImplementableEvent, Category = "EntityWindow")
    void OnTabChanged(int32 NewTabIndex);

    UFUNCTION(BlueprintImplementableEvent, Category = "EntityWindow")
    void OnActorBound(bool bHasActor);

private:
    int32 ActiveTabIndex = 0;
    FString CachedGrafanaUrl;
    FString CachedThingId;

    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    void BindToActor(ATempUIActor* Actor);
    void UnbindActor();
    void PopulateHeader();
    void SwitchToTab(int32 Index);

    UFUNCTION()
    void HandleCloseClicked();

    UFUNCTION()
    void HandleGrafanaClicked();

    UFUNCTION()
    void HandleTabInfoClicked();

    UFUNCTION()
    void HandleTabJsonClicked();

    UFUNCTION()
    void HandleTabAssocClicked();

    UFUNCTION()
    void HandleTabModelsClicked();
};
