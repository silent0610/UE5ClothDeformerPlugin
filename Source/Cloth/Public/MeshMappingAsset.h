#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "SparseMappingMatrix.h" // 包含我们核心的数据结构
#include "MeshMappingAsset.generated.h"

/**
 * @class UMeshMappingAsset
 * @brief 一个数据资产 (DataAsset)，用于在UE编辑器中存储
 * 预先计算好的稀疏映射矩阵 (FSparseMappingMatrix)。
 *
 * 这使得预计算的映射数据可以被序列化、保存到磁盘，
 * 并在运行时被GNN组件轻松引用和加载。
 */
UCLASS(BlueprintType) // 暴露给蓝图，以便在编辑器中创建和引用
class UMeshMappingAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    /**
     * @brief 存储的核心映射数据。
     *
     * 这个结构体 (FSparseMappingMatrix) 被直接嵌入到这个UDataAsset中。
     * * - 'VisibleAnywhere': 允许在资产编辑器的“Details”面板中查看数据（但不能编辑）。
     * - 'BlueprintReadOnly': 允许蓝图代码读取这个数据。
     * - 'Category': 将其组织在Details面板的一个名为“Mapping Data”的分类下。
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mapping Data")
    FSparseMappingMatrix MappingData{};
};