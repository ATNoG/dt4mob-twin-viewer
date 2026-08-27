#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/ModelsRowWidget.h"
#include "UI/ModelsGroupRowWidget.h"
#include "ModelsTabWidget.generated.h"

class ATempUIActor;
class UVerticalBox;
class UTextBlock;

UCLASS()
class DT4MOB_API UModelsTabWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "ModelsTab")
    void SetBoundActor(ATempUIActor* Actor);

protected:
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* LayerList;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* SectionLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ModelsTab")
    TSubclassOf<UModelsRowWidget> RowClass;

    /** Blueprint class instantiated for each mesh-layer group (e.g. "Cone"). Set in the Blueprint defaults. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ModelsTab")
    TSubclassOf<UModelsGroupRowWidget> GroupRowClass;

private:
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    /** @brief Group names currently expanded, so a full RebuildList() (e.g. triggered by any
     *  visibility/transparency toggle broadcasting OnMeshLayersChanged) can restore which groups
     *  were open instead of every freshly-recreated UModelsGroupRowWidget resetting to collapsed. */
    TSet<FString> ExpandedGroups;

    void RebuildList();

    UFUNCTION()
    void HandleMeshLayersChanged();

    UFUNCTION()
    void HandleGroupExpandedChanged(const FString& GroupName, bool bExpanded);
};
