#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "DropdownOptionWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOptionClicked, const FString&, TypeKey, const FString&, OptionDisplayName);

UCLASS()
class DT4MOB_API UDropdownOptionWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    virtual bool Initialize() override;

    UPROPERTY(BlueprintAssignable, BlueprintCallable)
    FOnOptionClicked OnOptionClicked;

    /** Sets the type key this option represents and updates the label to the key by default. */
    UFUNCTION(BlueprintCallable)
    void SetTypeKey(const FString& InTypeKey);

    /** Overrides the visible label without changing the internal type key. Call after SetTypeKey. */
    UFUNCTION(BlueprintCallable)
    void SetDisplayName(const FString& InDisplayName);

    /** Shows/hides the "no server handling" warning icon. */
    UFUNCTION(BlueprintCallable)
    void SetNoServerHandling(bool bValue);

    UFUNCTION(BlueprintPure)
    FString GetTypeKey() const { return TypeKey; }

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* OptionLabel;

    /** Must be named "OptionButton" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* OptionButton;

    /** Must be named "ButtonBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* ButtonBackground;

    /** "No server handling" warning icon. Must be named "Warningimage" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* Warningimage;

    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Dropdown")
    FString TypeKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Dropdown")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Dropdown")
    bool bNoServerHandling = false;

private:
    FLinearColor NormalColor = FLinearColor::Transparent;
    FLinearColor HoverColor = FLinearColor::Transparent;

    UFUNCTION()
    void HandleOptionButtonClicked();

    UFUNCTION()
    void HandleOptionButtonHovered();

    UFUNCTION()
    void HandleOptionButtonUnhovered();
};
