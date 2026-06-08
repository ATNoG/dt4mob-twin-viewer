#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "DropdownOptionWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOptionClicked, const FString&, TypeKey, const FString&, OptionDisplayName);

UCLASS()
class DT4MOB_API UDropdownOptionWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, BlueprintCallable)
    FOnOptionClicked OnOptionClicked;

    /** Sets the type key this option represents and updates the label to the key by default. */
    UFUNCTION(BlueprintCallable)
    void SetTypeKey(const FString& InTypeKey);

    /** Overrides the visible label without changing the internal type key. Call after SetTypeKey. */
    UFUNCTION(BlueprintCallable)
    void SetDisplayName(const FString& InDisplayName);

    UFUNCTION(BlueprintPure)
    FString GetTypeKey() const { return TypeKey; }

protected:
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* OptionLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Dropdown")
    FString TypeKey;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Dropdown")
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Dropdown")
    bool bNoServerHandling = false;
};
