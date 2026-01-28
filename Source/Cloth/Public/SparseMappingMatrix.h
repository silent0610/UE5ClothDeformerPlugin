#pragma once

#include "CoreMinimal.h"
#include "SparseMappingMatrix.generated.h" // 假设文件名

struct FTriplet
{
    int32 Row{}; // 低模顶点索引
    int32 Col{}; // 高模顶点索引
    float Value{};
    FTriplet(int32 InRow, int32 IntCol, float InValue);
};

USTRUCT(BlueprintType)
struct FSparseMappingMatrix
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, Category = "SparseMatrix")
    int32 NumRow;
    UPROPERTY(VisibleAnywhere, Category = "SparseMatrix")
    int32 NumCol;
    UPROPERTY(VisibleAnywhere, Category = "SparseMatrix")
    TArray<int32> RowPtr; // 行指针 (大小: NumRows + 1)

    UPROPERTY(VisibleAnywhere, Category = "SparseMatrix")
    TArray<int32> ColIndice; // 列索引 (大小: NumNonZeros)

    UPROPERTY(VisibleAnywhere, Category = "SparseMatrix")
    TArray<float> Value; // 权重值 (大小: NumNonZeros)

public:
    FSparseMappingMatrix(int32 InRow = 0, int32 InCol = 0);

    /**
     * @brief 策略构建的最后一步：将COO三元组列表转换为CSR
     * (O(N) 算法实现)
     */
    void SetFromTriplet(const TArray<FTriplet> &Triplets);
    /**
     * @brief 运行时核心运算：应用映射 (X_low = M * X_high)
     * @param InHighResVertices (X_high) GNN输出的高模顶点
     * @param OutLowResVertices (X_low) 映射生成的低模顶点
     */
    bool ApplyMapping(const TArray<FVector3f> &InHighResVertices, TArray<FVector3f> &OutLowResVertices) const;
};