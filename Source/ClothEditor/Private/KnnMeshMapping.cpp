#include "KnnMeshMapping.h"

#include "SparseMappingMatrix.h" // FTriplet, FSparseMappingMatrix
#include "DynamicMesh/DynamicMesh3.h"
#include "Widgets/Input/SSpinBox.h"
#include "MeshDescriptionToDynamicMesh.h"
#include "Spatial/PointHashGrid3.h"
#include "MeshMappingAsset.h"

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
    // [修正] Row 必须是高模顶点数，Col 必须是低模顶点数
    FSparseMappingMatrix mappingMatrix(HighPolyMesh->MaxVertexID(), LowPolyMesh->MaxVertexID());
    TArray<FTriplet> triplets;
    triplets.Reserve(HighPolyMesh->VertexCount() * k_);

    // --- Step 1. 在【低模】上建立空间索引 (我们要去低模里找点) ---
    FBox bounds = FBox(EForceInit::ForceInit);
    for (int32 vtxId : LowPolyMesh->VertexIndicesItr())
    {
        bounds += (FVector)LowPolyMesh->GetVertex(vtxId);
    }

    const double diagonal = (bounds.Max - bounds.Min).Length();
    const double cellSize = diagonal / FMath::Sqrt(static_cast<double>(LowPolyMesh->VertexCount()));

    UE::Geometry::TPointHashGrid3<int32, double> grid(cellSize, -1);
    grid.Reserve(LowPolyMesh->VertexCount());

    for (int32 vtxId : LowPolyMesh->VertexIndicesItr())
    {
        FVector3d pos = LowPolyMesh->GetVertex(vtxId);
        grid.InsertPointUnsafe(vtxId, pos);
    }

    // --- Step 2. 遍历【高模】查询K近邻并计算权重 ---
    for (int32 highId : HighPolyMesh->VertexIndicesItr())
    {
        FVector3d queryPos = HighPolyMesh->GetVertex(highId);

        TArray<int32> neighbors;
        TArray<double> distSq;
        neighbors.Reserve(k_);

        double radius = cellSize;
        while (neighbors.Num() < k_ && radius < diagonal)
        {
            grid.EnumeratePointsInBall(queryPos, radius,
                [&](int32 lowId) {
                    return (queryPos - (FVector3d)LowPolyMesh->GetVertex(lowId)).SquaredLength();
                },
                [&](int32 lowId, double d2) {
                    if (neighbors.Num() < k_)
                    {
                        neighbors.Add(lowId);
                        distSq.Add(d2);
                        return true; // continue
                    }
                    return false;
                });
            radius *= 2.0;
        }

        if (neighbors.Num() == 0)
        {
            continue;
        }

        // --- Step 3. 计算归一化权重 ---
        double sumW = 0;
        TArray<double> weights;
        weights.Reserve(neighbors.Num());

        for (double d2 : distSq)
        {
            double w = 1.0 / (d2 + kEpsilon_);
            weights.Add(w);
            sumW += w;
        }

        for (int32 j = 0; j < neighbors.Num(); ++j)
        {
            float normalized = static_cast<float>(weights[j] / sumW);
            // [修正] Row = 高模索引, Col = 低模索引, Value = 权重
            triplets.Emplace(highId, neighbors[j], normalized);
        }
    }

    // --- Step 4. 构建CSR矩阵 ---
    mappingMatrix.SetFromTriplet(triplets);
    return mappingMatrix;
}

bool FKnnMappingStrategy::GenerateMapping(
    const USkeletalMesh* LowPoly,
    const USkeletalMesh* HighPoly,
    UMeshMappingAsset* OutAsset
)
{
    // 1. 基本安全检查
    if (!LowPoly || !HighPoly || !OutAsset)
    {
        return false;
    }

    // 2. 从 SkeletalMesh 获取 LOD0 的基础网格描述 (MeshDescription)
    // 只有在 Editor 模块中才可以这样直接读取
    const FMeshDescription* lowDesc = LowPoly->GetMeshDescription(0);
    const FMeshDescription* highDesc = HighPoly->GetMeshDescription(0);

    if (!lowDesc || !highDesc)
    {
        return false;
    }

    // 3. 将 MeshDescription 转换为 GeometryCore 的 FDynamicMesh3
    UE::Geometry::FDynamicMesh3 lowDynMesh;
    FMeshDescriptionToDynamicMesh lowConverter;
    lowConverter.Convert(lowDesc, lowDynMesh);

    UE::Geometry::FDynamicMesh3 highDynMesh;
    FMeshDescriptionToDynamicMesh highConverter;
    highConverter.Convert(highDesc, highDynMesh);

    // 4. 调用核心算法计算映射矩阵
    FSparseMappingMatrix computedMatrix = BuildMappingMatrix(&highDynMesh, &lowDynMesh);

    // 5. 将结果写入资产，并标记为“已修改”(带上星号)，提示用户保存
    OutAsset->MappingData = computedMatrix;
    OutAsset->MarkPackageDirty();

    return true;
}

TSharedRef<SWidget> FKnnMappingStrategy::CreateSettingsWidget()
{
    // 这里定义 KNN 独有的参数界面
    return SNew(SVerticalBox)

        // --- 参数 K ---
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
                // 输入控件
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(SSpinBox<int32>) // 整数微调框
                        .Value(k_) // 或者直接设置初始值
                        .MinSliderValue(1)
                        .MaxSliderValue(10)
                        .OnValueChanged_Lambda([this](int32 NewValue)
                            {
                                // [核心] 当UI改变时，直接修改策略内部的成员变量
                                this->k_ = NewValue;
                            })
                ]
        ];
}
