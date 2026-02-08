#include "SurfaceProjection.h"

// --- 包含实现所需的UE模块 ---
#include "SparseMappingMatrix.h" // 用于 FTriplet 和 FSparseMappingMatrix
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAABBTree3.h"
#include "VectorTypes.h" // FVector3d, FIntVector3

using namespace UE::Geometry;

FString FSurfaceProjection::GetStrategyName() const
{
    return TEXT("Surface Projection (AABB)");
}

FSparseMappingMatrix FSurfaceProjection::BuildMappingMatrix(const FDynamicMesh3 *HighPolyMesh,
                                                            const FDynamicMesh3 *LowPolyMesh)
{
    FDynamicMeshAABBTree3 HighPolyTree{HighPolyMesh};
    const int32 NumLowVert = LowPolyMesh->MaxVertexID();
    const int32 NumHighVert = HighPolyMesh->MaxVertexID();

    TArray<FTriplet> Triplets;
    Triplets.Reserve(NumLowVert * 3);

    for (int32 VertexIndex = 0; VertexIndex < NumLowVert; ++VertexIndex)
    {
        if (!LowPolyMesh->IsVertex(VertexIndex))
        {
            continue;
        }

        const FVector3d QueryPoint = LowPolyMesh->GetVertex(VertexIndex);

        // 使用 FindNearestPoint 一步得到高模上最近点
        const FVector3d NearestPoint = HighPolyTree.FindNearestPoint(QueryPoint);

        // 获取最近三角形ID（FindNearestPoint内部已调用 FindNearestTriangle）
        double NearestDistSqr = 0.0;
        const int32 NearestTriID = HighPolyTree.FindNearestTriangle(QueryPoint, NearestDistSqr);
        if (NearestTriID != FDynamicMesh3::InvalidID)
        {
            // 直接计算最近点信息
            FDistPoint3Triangle3d DistQuery = TMeshQueries<FDynamicMesh3>::TriangleDistance(
                *HighPolyMesh, NearestTriID, QueryPoint);

            const FVector3d& NearestPos = DistQuery.ClosestTrianglePoint;
            const FVector3d& BaryCoord = DistQuery.TriangleBaryCoords; // 已经是对应权重

            const FIndex3i HighTriIndices = HighPolyMesh->GetTriangle(NearestTriID);

            Triplets.Add(FTriplet(VertexIndex, HighTriIndices.A, (float)BaryCoord.X));
            Triplets.Add(FTriplet(VertexIndex, HighTriIndices.B, (float)BaryCoord.Y));
            Triplets.Add(FTriplet(VertexIndex, HighTriIndices.C, (float)BaryCoord.Z));
        }
    }
    FSparseMappingMatrix ResultMatrix{NumLowVert, NumHighVert};
    ResultMatrix.SetFromTriplet(Triplets);
    return ResultMatrix;
}

bool FSurfaceProjection::GenerateMapping(
    const USkeletalMesh* LowPoly,
    const USkeletalMesh* HighPoly,
    UMeshMappingAsset* OutAsset
) {
    return false;
};
TSharedRef<SWidget> FSurfaceProjection::CreateSettingsWidget() {
    return SNew(SVerticalBox)

        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0, 5)
        [
            SNew(SHorizontalBox)
                // 标签
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(0, 0, 10, 0)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                        .Text(FText::FromString("K Value:"))
                        .ToolTipText(FText::FromString("Number of nearest neighbors to find"))
                ]
        ];
};