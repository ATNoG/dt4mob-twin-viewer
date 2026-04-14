#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinimapWidget.generated.h"

UCLASS()
class DT4MOB_API UMinimapWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, meta = (BindWidget))
    class UImage* MinimapImage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    UTexture2D* MinimapTexture;

    // Portugal bounds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    float NorthBound = 43.724381f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    float SouthBound = 36.711444f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    float WestBound = -11.600574f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
    float EastBound = -4.522815f;

protected:
    virtual int32 NativePaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const override;
    virtual void NativeConstruct() override;

private:
    FVector2D ECEFToMinimapUV(const FVector& ECEFPos, const FVector2D& WidgetSize) const;
    void GetPawnGroundCorners(TArray<FVector2D>& OutCorners, const FVector2D& WidgetSize) const;
};