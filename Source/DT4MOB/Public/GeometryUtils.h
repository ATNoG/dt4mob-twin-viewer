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

    /** Ramer-Douglas-Peucker line simplification: drops points that lie within ToleranceUnits
     *  of the straight segment connecting their neighbors, keeping the overall shape while
     *  cutting vertex count. Endpoints are always kept; input is treated as an open polyline
     *  (fine for closed rings too — pass the ring starting/ending at the same point, or just
     *  accept the first/last vertex is kept as-is). */
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
