#include "UI/MinimapWidget.h"
#include "Components/Image.h"
#include "Rendering/DrawElements.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/Texture2D.h"
#include "Engine/GameViewportClient.h"
#include "CesiumGeospatial/Ellipsoid.h"
#include "CesiumGeospatial/Cartographic.h"
#include "CesiumGeoreference.h"

FVector2D UMinimapWidget::ECEFToMinimapUV(const FVector& ECEFPos, const FVector2D& WidgetSize) const
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return FVector2D::ZeroVector;

    ACesiumGeoreference* Georeference = ACesiumGeoreference::GetDefaultGeoreference(PC->GetWorld());
    if (!Georeference) return FVector2D::ZeroVector;

    FVector LongLatHeight = Georeference->TransformUnrealPositionToLongitudeLatitudeHeight(ECEFPos);

    double Lon = LongLatHeight.X;
    double Lat = LongLatHeight.Y;

    UE_LOG(LogTemp, Warning, TEXT("Lat: %f, Lon: %f"), Lat, Lon);

    float U = FMath::Clamp((float)((Lon - WestBound) / (EastBound - WestBound)), 0.f, 1.f);
    float V = FMath::Clamp((float)(1.0 - (Lat - SouthBound) / (NorthBound - SouthBound)), 0.f, 1.f);

    return FVector2D(U * WidgetSize.X, V * WidgetSize.Y);
}

void UMinimapWidget::GetPawnGroundCorners(TArray<FVector2D>& OutCorners, const FVector2D& WidgetSize) const
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC) { UE_LOG(LogTemp, Warning, TEXT("No PC")); return; }
    if (!PC->GetPawn()) { UE_LOG(LogTemp, Warning, TEXT("No Pawn")); return; }

    FVector2D ViewportSize;
    GEngine->GameViewport->GetViewportSize(ViewportSize);
    UE_LOG(LogTemp, Warning, TEXT("ViewportSize: %s"), *ViewportSize.ToString());

    TArray<FVector2D> ScreenCorners = {
        { 0.f,            0.f            },
        { ViewportSize.X, 0.f            },
        { ViewportSize.X, ViewportSize.Y },
        { 0.f,            ViewportSize.Y },
    };

    FVector PawnLoc = PC->GetPawn()->GetActorLocation();
    FVector UpVector = PawnLoc.GetSafeNormal();
    UE_LOG(LogTemp, Warning, TEXT("PawnLoc: %s"), *PawnLoc.ToString());

    for (const FVector2D& SC : ScreenCorners)
    {
        FVector WorldOrigin, WorldDir;
        if (!PC->DeprojectScreenPositionToWorld(SC.X, SC.Y, WorldOrigin, WorldDir))
        { UE_LOG(LogTemp, Warning, TEXT("Deproject failed")); return; }

        float Denom = FVector::DotProduct(WorldDir, UpVector);
        if (FMath::IsNearlyZero(Denom, 0.001f)) { UE_LOG(LogTemp, Warning, TEXT("Denom zero")); return; }

        float T = FVector::DotProduct(PawnLoc - WorldOrigin, UpVector) / Denom;
        if (T < 0.f) { UE_LOG(LogTemp, Warning, TEXT("T negative: %f"), T); return; }

        FVector GroundHit = WorldOrigin + WorldDir * T;
        OutCorners.Add(ECEFToMinimapUV(GroundHit, WidgetSize));
    }
}


int32 UMinimapWidget::NativePaint(
    const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    int32 Layer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect,
        OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

    FVector2D WidgetSize = AllottedGeometry.GetLocalSize();

    APlayerController* PC = GetOwningPlayer();
    if (!PC || !PC->GetPawn()) return Layer;

    FVector PawnLoc = PC->GetPawn()->GetActorLocation();
    FVector2D MinimapPos = ECEFToMinimapUV(PawnLoc, WidgetSize);
    UE_LOG(LogTemp, Warning, TEXT("PawnLoc ECEF: %s"), *PawnLoc.ToString());
    UE_LOG(LogTemp, Warning, TEXT("MinimapPos: %s"), *MinimapPos.ToString());
    UE_LOG(LogTemp, Warning, TEXT("WidgetSize: %s"), *WidgetSize.ToString());

    TArray<FVector2D> DotPoints;
    float R = 4.f;
    for (int i = 0; i <= 8; i++)
    {
        float A = (float)i / 8.f * 2.f * PI;
        DotPoints.Add(MinimapPos + FVector2D(FMath::Cos(A), FMath::Sin(A)) * R);
    }

    FSlateDrawElement::MakeLines(OutDrawElements, ++Layer,
        AllottedGeometry.ToPaintGeometry(),
        DotPoints, ESlateDrawEffect::None,
        FLinearColor::White, true, 2.f);

    return Layer;
}

void UMinimapWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (MinimapTexture && MinimapImage)
    {
        FSlateBrush Brush;
        Brush.SetResourceObject(MinimapTexture);
        Brush.ImageSize = FVector2D(MinimapTexture->GetSizeX(), MinimapTexture->GetSizeY());
        MinimapImage->SetBrush(Brush);
    }
}
