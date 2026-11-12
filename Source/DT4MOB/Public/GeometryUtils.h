#pragma once

/**
 * @file
 * @brief Header-only 2D geometry helpers (convex hull, polyline simplification).
 * @ingroup core
 */

#include "CoreMinimal.h"

/**
 * @brief Static 2D geometry helpers with no UObject dependency.
 *
 * Header-only, so any struct, component or service can include it. Used for terrain exclusion
 * hulls around entity meshes and for simplifying fire simulation outlines.
 *
 * @ingroup core
 */
struct FGeometryUtils
{
    /**
     * @brief Computes the convex hull of a 2D point set with Andrew's monotone chain algorithm.
     *
     * Points that lie on a hull edge (collinear) are dropped, so only the corner vertices remain.
     * Runs in O(n log n) because of the initial sort.
     *
     * @param Points  Input points, taken by value because the function sorts its own copy.
     * @return        Hull vertices in counter-clockwise order (X right, Y up), without repeating
     *                the first vertex at the end. With fewer than 3 input points, returns the
     *                input sorted by X and then Y.
     */
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

    /**
     * @brief Simplifies a polyline with the Ramer-Douglas-Peucker algorithm.
     *
     * Recursively keeps the point farthest from the segment between the current endpoints
     * while that distance exceeds ToleranceUnits, and drops the rest. Iterative (explicit
     * stack), so long inputs cannot overflow the call stack. The input is treated as an open
     * polyline and both endpoints are always kept. A closed ring also works: pass it with the
     * first point repeated at the end, and the shared start/end vertex is kept as-is.
     *
     * @param Points          Polyline vertices in order.
     * @param ToleranceUnits  Maximum allowed deviation, in the same units as Points (for
     *                        example degrees when the points are lon/lat).
     * @return                The kept vertices in their original order. Returns Points
     *                        unchanged when it has fewer than 3 vertices or ToleranceUnits <= 0.
     */
    static TArray<FVector2D> SimplifyPolyline2D(const TArray<FVector2D>& Points, double ToleranceUnits)
    {
        const int32 N = Points.Num();
        if (N < 3 || ToleranceUnits <= 0.0)
            return Points;

        TArray<bool> Keep;
        Keep.Init(false, N);
        Keep[0] = true;
        Keep[N - 1] = true;

        TArray<TPair<int32, int32>> Stack;
        Stack.Push(TPair<int32, int32>(0, N - 1));
        while (Stack.Num() > 0)
        {
            const TPair<int32, int32> Range = Stack.Pop();
            const int32 StartIdx = Range.Key, EndIdx = Range.Value;
            if (EndIdx <= StartIdx + 1)
                continue;

            const FVector2D& A = Points[StartIdx];
            const FVector2D& B = Points[EndIdx];
            const FVector2D AB = B - A;
            const double ABLenSq = AB.SizeSquared();

            double MaxDist = 0.0;
            int32 MaxIdx = INDEX_NONE;
            for (int32 i = StartIdx + 1; i < EndIdx; i++)
            {
                const FVector2D& P = Points[i];
                double Dist;
                if (ABLenSq < UE_KINDA_SMALL_NUMBER)
                {
                    Dist = FVector2D::Distance(P, A);
                }
                else
                {
                    const double T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / ABLenSq, 0.0, 1.0);
                    Dist = FVector2D::Distance(P, A + AB * T);
                }
                if (Dist > MaxDist)
                {
                    MaxDist = Dist;
                    MaxIdx = i;
                }
            }

            if (MaxIdx != INDEX_NONE && MaxDist > ToleranceUnits)
            {
                Keep[MaxIdx] = true;
                Stack.Push(TPair<int32, int32>(StartIdx, MaxIdx));
                Stack.Push(TPair<int32, int32>(MaxIdx, EndIdx));
            }
        }

        TArray<FVector2D> Out;
        Out.Reserve(N);
        for (int32 i = 0; i < N; i++)
            if (Keep[i])
                Out.Add(Points[i]);
        return Out;
    }
};
