#pragma once

#include "CoreMinimal.h"
#include "SparseMappingMatrix.generated.h" // 假设文件名

struct CLOTH_API FTriplet
{
    int32 Row{}; // 高模顶点索引
    int32 Col{}; // 低模顶点索引
    float Value{};
    FTriplet(int32 InRow, int32 IntCol, float InValue);
};

USTRUCT(BlueprintType)
struct CLOTH_API FSparseMappingMatrix
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


    /*
    CSR, 三个数组, 对缓存友好
	RotPtr, 如 0,3,7, 表示第一行有3个非零元素，第二行有4个非零元素. 即前三个顶点的权重来自第0行，接下来四个顶点的权重来自第1行
	ColIndice, 如 0,2,5, 7,8,9,10 代表对应行的非零元素在低模顶点索引. 比如第一行的3个非零元素分别来自低模顶点0,2,5. 第二行的4个非零元素分别来自低模顶点7,8,9,10
	Value, 如 0.5,0.3,0.2, 0.4,0.4,0.1,0.1 代表对应行的非零元素的权重值. 比如第一行的3个非零元素分别是0.5,0.3,0.2，第二行的4个非零元素分别是0.4,0.4,0.1,0.1
    */

public:
    FSparseMappingMatrix(int32 InRow = 0, int32 InCol = 0);

    /**
     * @brief 策略构建的最后一步：将COO三元组列表转换为CSR
     * (O(N) 算法实现)
     */
    void SetFromTriplet(const TArray<FTriplet> &Triplets);

    // 从低模映射到高模
    bool ApplyMapping(const TArray<FVector> &InLowResOffsets, TArray<FVector> &OutHighResOffsets) const;
};