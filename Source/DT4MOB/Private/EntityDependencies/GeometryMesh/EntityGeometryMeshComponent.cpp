// Fill out your copyright notice in the Description page of Project Settings.

#include "EntityDependencies/GeometryMesh/EntityGeometryMeshComponent.h"
#include "Entities/TempUIActor.h"
#include "Services/CoordinatesConversionService.h"
#include "ProceduralMeshComponent.h"
#include "GeomTools.h"
#include "UObject/UnrealType.h"

void UEntityGeometryMeshComponent::OnEntityInitialized()
{
    RebuildMesh();
}

void UEntityGeometryMeshComponent::OnEntityDataChanged()
{
    RebuildMesh();
}

bool UEntityGeometryMeshComponent::ExtractGeometry(const TSharedPtr<FStructOnScope>& StructInstance, TArray<FGeoPoint>& OutPoints, double& OutShapeLength, double& OutShapeArea)
{
    OutPoints.Reset();
    OutShapeLength = 0.0;
    OutShapeArea = 0.0;

    if (!StructInstance.IsValid())
        return false;

    const UScriptStruct* RootStruct = Cast<UScriptStruct>(StructInstance->GetStruct());
    if (!RootStruct)
        return false;

    for (TFieldIterator<FProperty> It(RootStruct); It; ++It)
    {
        if (!It->GetName().Equals(TEXT("attributes"), ESearchCase::IgnoreCase))
            continue;

        FStructProperty* AttribProp = CastField<FStructProperty>(*It);
        if (!AttribProp)
            continue;

        void* AttribPtr = AttribProp->ContainerPtrToValuePtr<void>(StructInstance->GetStructMemory());
        UScriptStruct* AttribStruct = AttribProp->Struct;

        bool bFoundGeometry = false;

        for (TFieldIterator<FProperty> AttrIt(AttribStruct); AttrIt; ++AttrIt)
        {
            const FString AttrName = AttrIt->GetName();

            if (AttrName.Equals(TEXT("geometry"), ESearchCase::IgnoreCase))
            {
                FArrayProperty* ArrayProp = CastField<FArrayProperty>(*AttrIt);
                FStructProperty* ElemStructProp = ArrayProp ? CastField<FStructProperty>(ArrayProp->Inner) : nullptr;
                if (!ArrayProp || !ElemStructProp)
                    continue;

                void* ArrayPtr = ArrayProp->ContainerPtrToValuePtr<void>(AttribPtr);
                FScriptArrayHelper ArrayHelper(ArrayProp, ArrayPtr);

                OutPoints.Reserve(ArrayHelper.Num());
                for (int32 i = 0; i < ArrayHelper.Num(); ++i)
                {
                    void* ElemPtr = ArrayHelper.GetRawPtr(i);
                    FGeoPoint Point;

                    for (TFieldIterator<FProperty> PtIt(ElemStructProp->Struct); PtIt; ++PtIt)
                    {
                        FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(*PtIt);
                        if (!DoubleProp)
                            continue;

                        const FString PtName = PtIt->GetName();
                        if (PtName.Equals(TEXT("latitude"), ESearchCase::IgnoreCase))
                            Point.Latitude = *(double*)DoubleProp->ContainerPtrToValuePtr<void>(ElemPtr);
                        else if (PtName.Equals(TEXT("longitude"), ESearchCase::IgnoreCase))
                            Point.Longitude = *(double*)DoubleProp->ContainerPtrToValuePtr<void>(ElemPtr);
                    }

                    OutPoints.Add(Point);
                }

                bFoundGeometry = true;
            }
            else if (AttrName.Contains(TEXT("shape_length"), ESearchCase::IgnoreCase))
            {
                if (FDoubleProperty* P = CastField<FDoubleProperty>(*AttrIt))
                    OutShapeLength = *(double*)P->ContainerPtrToValuePtr<void>(AttribPtr);
            }
            else if (AttrName.Contains(TEXT("shape_area"), ESearchCase::IgnoreCase))
            {
                if (FDoubleProperty* P = CastField<FDoubleProperty>(*AttrIt))
                    OutShapeArea = *(double*)P->ContainerPtrToValuePtr<void>(AttribPtr);
            }
        }

        return bFoundGeometry;
    }

    return false;
}

void UEntityGeometryMeshComponent::BuildFilledMesh(const TArray<FVector>& LocalPoints, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles, TArray<FVector>& OutNormals, TArray<FVector2D>& OutUVs)
{
    TArray<FVector2D> Ring2D;
    Ring2D.Reserve(LocalPoints.Num());
    for (const FVector& P : LocalPoints)
        Ring2D.Add(FVector2D(P.X, P.Y));

    // TriangulatePoly expects a simple polygon, not an explicitly closed ring — drop a duplicated closing point.
    if (Ring2D.Num() > 2 && FVector2D::DistSquared(Ring2D[0], Ring2D.Last()) < 1.0)
        Ring2D.RemoveAt(Ring2D.Num() - 1);

    if (Ring2D.Num() < 3)
        return;

    TArray<FVector2D> Tris2D;
    if (!FGeomTools2D::TriangulatePoly(Tris2D, Ring2D, false))
        return;

    OutVertices.Reserve(Tris2D.Num());
    OutTriangles.Reserve(Tris2D.Num());
    OutNormals.Reserve(Tris2D.Num());
    OutUVs.Reserve(Tris2D.Num());

    for (int32 i = 0; i < Tris2D.Num(); ++i)
    {
        OutVertices.Add(FVector(Tris2D[i].X, Tris2D[i].Y, MeshHeightCm));
        OutTriangles.Add(i);
        OutNormals.Add(FVector::UpVector);
        OutUVs.Add(Tris2D[i] / 100.f);
    }
}

void UEntityGeometryMeshComponent::BuildRibbonMesh(const TArray<FVector>& LocalPoints, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles, TArray<FVector>& OutNormals, TArray<FVector2D>& OutUVs)
{
    if (LocalPoints.Num() < 2)
        return;

    OutVertices.Reserve(LocalPoints.Num() * 2);
    OutNormals.Reserve(LocalPoints.Num() * 2);
    OutUVs.Reserve(LocalPoints.Num() * 2);

    for (int32 i = 0; i < LocalPoints.Num(); ++i)
    {
        FVector2D Dir;
        if (i == 0)
            Dir = FVector2D(LocalPoints[1] - LocalPoints[0]).GetSafeNormal();
        else if (i == LocalPoints.Num() - 1)
            Dir = FVector2D(LocalPoints[i] - LocalPoints[i - 1]).GetSafeNormal();
        else
            Dir = FVector2D(LocalPoints[i + 1] - LocalPoints[i - 1]).GetSafeNormal();

        const FVector2D Perp(-Dir.Y, Dir.X);
        const FVector Base(LocalPoints[i].X, LocalPoints[i].Y, MeshHeightCm);
        const FVector Offset(Perp.X * RibbonHalfWidthCm, Perp.Y * RibbonHalfWidthCm, 0.f);

        OutVertices.Add(Base + Offset);
        OutVertices.Add(Base - Offset);
        OutNormals.Add(FVector::UpVector);
        OutNormals.Add(FVector::UpVector);
        OutUVs.Add(FVector2D(0.f, (float)i));
        OutUVs.Add(FVector2D(1.f, (float)i));
    }

    OutTriangles.Reserve((LocalPoints.Num() - 1) * 6);
    for (int32 i = 0; i < LocalPoints.Num() - 1; ++i)
    {
        const int32 L0 = i * 2;
        const int32 R0 = i * 2 + 1;
        const int32 L1 = (i + 1) * 2;
        const int32 R1 = (i + 1) * 2 + 1;

        OutTriangles.Add(L0); OutTriangles.Add(L1); OutTriangles.Add(R0);
        OutTriangles.Add(R0); OutTriangles.Add(L1); OutTriangles.Add(R1);
    }
}

void UEntityGeometryMeshComponent::RebuildMesh()
{
    if (!bMeshGenerationEnabled)
        return;

    ATempUIActor* Owner = GetOwnerEntity();
    if (!Owner || !Owner->StructInstance.IsValid())
        return;

    TArray<FGeoPoint> Points;
    double ShapeLength = 0.0;
    double ShapeArea = 0.0;
    if (!ExtractGeometry(Owner->StructInstance, Points, ShapeLength, ShapeArea) || Points.Num() < 2)
        return;

    UCoordinatesConversionService* CoordSvc = UCoordinatesConversionService::Get();
    const FVector OwnerLocation = Owner->GetActorLocation();

    TArray<FVector> LocalPoints;
    LocalPoints.Reserve(Points.Num());
    for (const FGeoPoint& Pt : Points)
    {
        const FVector WorldPt = CoordSvc->ConvertWSG84ToUELocal(Pt.Latitude, Pt.Longitude, 0.0);
        LocalPoints.Add(WorldPt - OwnerLocation);
    }

    const bool bClosed = ShapeArea != 0.0 ||
        (LocalPoints.Num() > 2 && FVector::DistSquared(LocalPoints[0], LocalPoints.Last()) < 1.0);

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;

    if (bClosed)
        BuildFilledMesh(LocalPoints, Vertices, Triangles, Normals, UVs);
    else
        BuildRibbonMesh(LocalPoints, Vertices, Triangles, Normals, UVs);

    if (Vertices.IsEmpty())
        return;

    if (!MeshComponent)
    {
        MeshComponent = NewObject<UProceduralMeshComponent>(Owner, TEXT("GeometryMesh"));
        MeshComponent->SetupAttachment(Owner->GetRootComponent());
        MeshComponent->RegisterComponent();
    }

    const TArray<FColor> EmptyColors;
    const TArray<FProcMeshTangent> EmptyTangents;
    MeshComponent->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, EmptyColors, EmptyTangents, false);
}
