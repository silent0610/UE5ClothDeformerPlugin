#include "KnnMeshMapping.h"

#include "SparseMappingMatrix.h" // FTriplet, FSparseMappingMatrix
#include "DynamicMesh/DynamicMesh3.h"


#include "Spatial/PointHashGrid3.h"

FKnnMappingStrategy::FKnnMappingStrategy(int32 k)
    : k_(k), kEpsilon_(1e-8f)
{
    ;
}
FString FKnnMappingStrategy::GetStrategyName() const
{
    return FString::Printf(TEXT("KNN"));
}

FSparseMappingMatrix FKnnMappingStrategy::BuildMappingMatrix(const UE::Geometry::FDynamicMesh3 *HighPolyMesh, const UE::Geometry::FDynamicMesh3 *LowPolyMesh)
{
    FSparseMappingMatrix Mapping(LowPolyMesh->MaxVertexID(), HighPolyMesh->MaxVertexID());
    TArray<FTriplet> Triplets;
    Triplets.Reserve(LowPolyMesh->VertexCount() * k_);

    // --- Step 1. 建立空间索引 ---
    // 自动推导CellSize
    FBox Bounds = FBox(EForceInit::ForceInit);
    for (int32 VtxID : HighPolyMesh->VertexIndicesItr())
    {
        Bounds += (FVector)HighPolyMesh->GetVertex(VtxID);
    }
    const double Diagonal = (Bounds.Max - Bounds.Min).Length();
    const double CellSize = Diagonal / FMath::Sqrt(static_cast<double>(HighPolyMesh->VertexCount()));

    UE::Geometry::TPointHashGrid3<int32, double> Grid(CellSize, -1);
    Grid.Reserve(HighPolyMesh->VertexCount());

    for (int32 VtxID : HighPolyMesh->VertexIndicesItr())
    {
        FVector3d Pos = HighPolyMesh->GetVertex(VtxID);
        Grid.InsertPointUnsafe(VtxID, Pos);
    }

    // --- Step 2. 查询K近邻并计算权重 ---
    for (int32 LowID : LowPolyMesh->VertexIndicesItr())
    {
        FVector3d QueryPos = LowPolyMesh->GetVertex(LowID);

        TArray<int32> Neighbors;
        TArray<double> DistSq;
        Neighbors.Reserve(k_);

        // 逐步扩大搜索半径直到找到足够邻居
        double Radius = CellSize;
        while (Neighbors.Num() < k_ && Radius < Diagonal)
        {
            Grid.EnumeratePointsInBall(QueryPos, Radius,
                [&](int32 HighID) {
                    return (QueryPos - (FVector3d)HighPolyMesh->GetVertex(HighID)).SquaredLength();
                },
                [&](int32 HighID, double D2) {
                    if (Neighbors.Num() < k_)
                    {
                        Neighbors.Add(HighID);
                        DistSq.Add(D2);
                        return true; // continue
                    }
                    return false;
                });
            Radius *= 2.0;
        }

        if (Neighbors.Num() == 0)
            continue;

        // --- Step 3. 计算归一化权重 ---
        double SumW = 0;
        TArray<double> Weights;
        Weights.Reserve(Neighbors.Num());

        for (double D2 : DistSq)
        {
            double W = 1.0 / (D2 + kEpsilon_);
            Weights.Add(W);
            SumW += W;
        }

        for (int32 j = 0; j < Neighbors.Num(); ++j)
        {
            float Normalized = static_cast<float>(Weights[j] / SumW);
            Triplets.Emplace(LowID, Neighbors[j], Normalized);
        }
    }

    // --- Step 4. 构建CSR矩阵 ---
    Mapping.SetFromTriplet(Triplets);
    return Mapping;
}