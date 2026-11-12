/**
 * @file
 * @brief Declares UInfoTabWidget, the Info tab of the entity window.
 * @ingroup ui
 */
#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "UI/InfoFieldTypes.h"
#include "InfoTabWidget.generated.h"

class ATempUIActor;
class UVerticalBox;
class UTextBlock;
class UButton;
class UBorder;
class UInfoFieldRegistry;

/** @brief Fired when the user clicks the Info tab's configure button. @ingroup ui */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInfoConfigureRequested);

/**
 * @brief Info tab of the entity window: a label/value row for each field selected for the
 *        entity's type.
 *
 * The fields come from UInfoFieldRegistry for the type of the bound actor. Values are read
 * from the actor's raw JSON and refreshed whenever the actor's data changes. The rows are
 * rebuilt when the field selection for the type changes.
 *
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UInfoTabWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /** @brief Subscribes to field selection changes and binds the configure button. */
    virtual void NativeConstruct() override;

    /** @brief Unsubscribes from field selection changes and unbinds the actor. */
    virtual void NativeDestruct() override;

    /**
     * @brief Shows the given entity, replacing the previous one, and rebuilds the rows.
     * @param Actor Entity to show, or null to clear the tab.
     */
    UFUNCTION(BlueprintCallable, Category = "InfoTab")
    void SetBoundActor(ATempUIActor* Actor);

    /** @brief Fired when the user clicks the configure button: EntityWindowWidget handles the panel. */
    UPROPERTY(BlueprintAssignable, Category = "InfoTab")
    FOnInfoConfigureRequested OnConfigureRequested;

protected:
    /**
     * @brief Applies theme colours to the header, the configure button and the row text.
     * @param Theme Active theme.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

    /** @brief Container for the generated rows. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UVerticalBox* PropertyList;

    /** @brief Optional button that opens the field selection panel. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UButton* ConfigureBtn;

    /** @brief Optional border behind ConfigureBtn that shows hover and press colours. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* ConfigBtnBorder;

    /** @brief Section heading ("Properties"). Must be named "HeaderLabel" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* HeaderLabel;

    /** @brief ConfigureBtn's label ("Config Panel"). Must be named "ConfigureBtnLabel" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UTextBlock* ConfigureBtnLabel;

    /** @brief Font for the row label (left side). Set in Blueprint defaults. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InfoTab")
    FSlateFontInfo LabelFont;

    /** @brief Font for the row value (right side). Set in Blueprint defaults. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InfoTab")
    FSlateFontInfo ValueFont;

    /** @brief Color for row labels. Overwritten by the theme's secondary text colour in ApplyTheme. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InfoTab")
    FLinearColor LabelColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.f);  // ~#7a7a7a

    /** @brief Color for row values. Overwritten by the theme's primary text colour in ApplyTheme. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "InfoTab")
    FLinearColor ValueColor = FLinearColor(0.72f, 0.72f, 0.72f, 1.f); // ~#dcdcdc

private:
    /** @brief Entity currently shown. */
    UPROPERTY()
    ATempUIActor* BoundActor = nullptr;

    /** @brief Entity type of BoundActor, looked up from the factory. */
    FString CachedTypeKey;

    /** @brief Label text blocks of the current rows. */
    UPROPERTY()
    TArray<UTextBlock*> LabelBlocks;

    /** @brief Value text blocks of the current rows, in the same order as LabelBlocks. */
    UPROPERTY()
    TArray<UTextBlock*> ValueBlocks;

    /** @brief Applies LabelColor and ValueColor to the existing rows. Call from Blueprint's ApplyTheme after updating LabelColor/ValueColor. */
    UFUNCTION(BlueprintCallable, Category = "InfoTab")
    void RefreshColors();

    /** @brief Recreates one row per selected field for the bound actor's type, then fills the values. */
    void RebuildRows();

    /** @brief Re-reads every row value from the bound actor's JSON. */
    void RefreshValues();

    /** @brief Refreshes the values after the bound actor's data changed. */
    UFUNCTION()
    void HandleEntityDataChanged();

    /**
     * @brief Rebuilds the rows if the field selection changed for the shown type.
     * @param TypeKey Type whose selection changed.
     */
    UFUNCTION()
    void HandleFieldsChanged(const FString& TypeKey);

    /** @brief Fires OnConfigureRequested. */
    UFUNCTION()
    void HandleConfigureClicked();

    UFUNCTION()
    void HandleConfigureBtnHovered();   ///< Sets the button border to the hover colour.

    UFUNCTION()
    void HandleConfigureBtnUnhovered(); ///< Sets the button border back to the normal colour.

    UFUNCTION()
    void HandleConfigureBtnPressed();   ///< Sets the button border to the pressed colour.

    UFUNCTION()
    void HandleConfigureBtnReleased();  ///< Sets the button border back to the hover colour.

    FLinearColor BorderColorNormal;  ///< Button border colour at rest, from the theme.
    FLinearColor BorderColorHovered; ///< Button border colour on hover, from the theme.
    FLinearColor BorderColorPressed; ///< Button border colour while pressed, from the theme.

    /**
     * @brief Finds the field registry of the owning local player.
     * @return The registry, or null if there is no owning player.
     */
    UInfoFieldRegistry* GetRegistry() const;
};
