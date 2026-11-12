/**
 * @file
 * @brief Declares UDropdownOptionWidget, one entry in the entity type dropdown.
 * @ingroup ui
 */

#pragma once

#include "CoreMinimal.h"
#include "UI/ThemedWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "DropdownOptionWidget.generated.h"

/**
 * @brief Fired when a dropdown option is clicked.
 * @param TypeKey            Entity type key of the option. Empty for the "None" entry.
 * @param OptionDisplayName  The option's DisplayName property (empty unless it was set on spawn).
 * @ingroup ui
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOptionClicked, const FString&, TypeKey, const FString&, OptionDisplayName);

/**
 * @brief One clickable entry in the popup list of UEntityTypeDropdownWidget.
 *
 * Shows a type's name, an optional warning icon for types with no server-side handling,
 * and a hover highlight. Clicking it broadcasts OnOptionClicked.
 * @ingroup ui
 */
UCLASS()
class DT4MOB_API UDropdownOptionWidget : public UThemedWidget
{
    GENERATED_BODY()

public:
    /**
     * @brief Binds the option button's click and hover events.
     * @return False if the base class fails to initialize.
     */
    virtual bool Initialize() override;

    /** @brief Broadcast with TypeKey and DisplayName when the option button is clicked. */
    UPROPERTY(BlueprintAssignable, BlueprintCallable)
    FOnOptionClicked OnOptionClicked;

    /**
     * @brief Sets the type key this option represents and shows the key as the label.
     * @param InTypeKey  Factory type key. Empty means "None", which is also what the label shows.
     */
    UFUNCTION(BlueprintCallable)
    void SetTypeKey(const FString& InTypeKey);

    /**
     * @brief Overrides the visible label without changing the type key. Call after SetTypeKey().
     * @param InDisplayName  Text to show. Empty shows "None".
     * @note Only the label changes. The DisplayName property sent with OnOptionClicked is not updated.
     */
    UFUNCTION(BlueprintCallable)
    void SetDisplayName(const FString& InDisplayName);

    /**
     * @brief Shows or hides the "no server handling" warning icon.
     * @param bValue  True to show the icon.
     */
    UFUNCTION(BlueprintCallable)
    void SetNoServerHandling(bool bValue);

    /** @return The type key set by SetTypeKey(). Empty for the "None" entry. */
    UFUNCTION(BlueprintPure)
    FString GetTypeKey() const { return TypeKey; }

protected:
    /** @brief Text that shows the option's name. Set in the widget Blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UTextBlock* OptionLabel;

    /** @brief Clickable area of the option. Must be named "OptionButton" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    UButton* OptionButton;

    /** @brief Background recolored on hover. Must be named "ButtonBackground" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UBorder* ButtonBackground;

    /** @brief "No server handling" warning icon. Must be named "Warningimage" in the Blueprint layout. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    UImage* Warningimage;

    /**
     * @brief Stores the normal and hover background colors and colors the label.
     * @param Theme  Active theme. Ignored if null.
     */
    virtual void ApplyTheme_Implementation(UUIThemeData* Theme) override;

    /** @brief Type key this option represents. Exposed on spawn; normally set through SetTypeKey(). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Dropdown")
    FString TypeKey;

    /** @brief Display name broadcast with OnOptionClicked. Only set on spawn or in the editor; SetDisplayName() does not write it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Dropdown")
    FString DisplayName;

    /** @brief True if the type has no server-side handling. Updated by SetNoServerHandling(). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = "Dropdown")
    bool bNoServerHandling = false;

private:
    /** @brief Background color when not hovered (theme BackgroundPrimary). */
    FLinearColor NormalColor = FLinearColor::Transparent;

    /** @brief Background color while hovered (theme Hover). */
    FLinearColor HoverColor = FLinearColor::Transparent;

    /** @brief Broadcasts OnOptionClicked. */
    UFUNCTION()
    void HandleOptionButtonClicked();

    /** @brief Switches the background to the hover color. */
    UFUNCTION()
    void HandleOptionButtonHovered();

    /** @brief Switches the background back to the normal color. */
    UFUNCTION()
    void HandleOptionButtonUnhovered();
};
