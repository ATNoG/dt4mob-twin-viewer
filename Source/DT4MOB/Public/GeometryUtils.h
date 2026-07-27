#pragma once

#include "CoreMinimal.h"

/**
 * Lightweight 2D geometry utilities.
 * No UObject dependency — safe to include from any struct or component.
 */
struct FGeometryUtils
{
    /** Andrew's monotone chain: returns the convex hull of Points in counter-clockwise order. */
    static TArray<FVector2D> ComputeConvexHull2D(TArray<FVector2D> Points)
    {
        Points.Sort([](const FVector2D& A, const FVector2D& B)
        {
            return A.X != B.X ? A.X < B.X : A.Y < B.Y;
        });

        const int32 N = Points.Num();
        if (N < 3)
            return Points;

        auto Cross = [](const FVector2D& O, const FVector2D& A, const FVector2D& B)
        {
            return (A.X - O.X) * (B.Y - O.Y) - (A.Y - O.Y) * (B.X - O.X);
        };

        TArray<FVector2D> Hull;
        Hull.SetNum(2 * N);
        int32 K = 0;

        // Lower hull.
        for (int32 i = 0; i < N; i++)
        {
            while (K >= 2 && Cross(Hull[K - 2], Hull[K - 1], Points[i]) <= 0)
                K--;
            Hull[K++] = Points[i];
        }

        // Upper hull.
        for (int32 i = N - 2, LowerCount = K + 1; i >= 0; i--)
        {
            while (K >= LowerCount && Cross(Hull[K - 2], Hull[K - 1], Points[i]) <= 0)
                K--;
            Hull[K++] = Points[i];
        }

        Hull.SetNum(K - 1); // last point duplicates the first
        return Hull;
    }
};
