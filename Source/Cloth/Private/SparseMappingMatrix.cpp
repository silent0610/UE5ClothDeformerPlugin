#include "SparseMappingMatrix.h"

FTriplet::FTriplet(int32 InRow, int32 InCol, float InValue) : Row(InRow), Col(InCol), Value(InValue) { ; }

FSparseMappingMatrix::FSparseMappingMatrix(int32 InRow, int32 InCol) : NumRow(InRow), NumCol(InCol)
{
    ;
}
void FSparseMappingMatrix::SetFromTriplet(const TArray<FTriplet> &Triplets)
{
    int32 numNonZeros = Triplets.Num();

    // 预分配空间
    ColIndice.SetNumUninitialized(numNonZeros);
    Value.SetNumUninitialized(numNonZeros);

    // RowPtrs 大小必须是 NumRows + 1，初始全为0
    RowPtr.Init(0, NumRow + 1);

    // 第一步：统计每一行有多少个非零元素
    for (const FTriplet& triplet : Triplets)
    {
        // triplet.Row 必须在合法范围内
        if (triplet.Row >= 0 && triplet.Row < NumRow)
        {
            RowPtr[triplet.Row + 1]++;
        }
    }

    // 第二步：计算前缀和，将 RowPtrs 转化为真正的偏移量索引
    for (int32 i = 0; i < NumRow; ++i)
    {
        RowPtr[i + 1] += RowPtr[i];
    }

    // 第三步：填充 ColIndices 和 Values 数据
    // 使用 currentRowOffsets 来记录当前行写到了哪个位置
    TArray<int32> currentRowOffsets = RowPtr;
    for (const FTriplet& triplet : Triplets)
    {
        if (triplet.Row >= 0 && triplet.Row < NumRow)
        {
            int32 insertIndex = currentRowOffsets[triplet.Row]++;
            ColIndice[insertIndex] = triplet.Col;
            Value[insertIndex] = triplet.Value;
        }
    }
}

// 从低模映射到低模
bool FSparseMappingMatrix::ApplyMapping(const TArray<FVector>& InLowResOffsets, TArray<FVector>& OutHighResOffsets) const
{
    // 1. 安全检查：输入的低模位移数量，必须精确等于矩阵的列数 (NumCol)
    if (InLowResOffsets.Num() != NumCol)
    {
        return false;
    }

    // 2. 初始化输出数组：大小设为高模顶点数 (NumRow)，并全部填充为 0
    OutHighResOffsets.Init(FVector::ZeroVector, NumRow);

    // 3. 核心计算：遍历高模的每一个顶点（矩阵的每一行）
    for (int32 row = 0; row < NumRow; ++row)
    {
        FVector accumulatedOffset = FVector::ZeroVector;

        // [第1步：查字典] 获取当前行在 ColIndice 和 Value 数组中的起止范围
        const int32 startIdx = RowPtr[row];
        const int32 endIdx = RowPtr[row + 1];

        // [第2步：顺藤摸瓜] 遍历影响该高模顶点的所有低模顶点数据
        for (int32 i = startIdx; i < endIdx; ++i)
        {
            const int32 col = ColIndice[i];
            const float weight = Value[i];

            // [第3步：加权求和] 低模顶点的位移 * 对应的权重
            accumulatedOffset += InLowResOffsets[col] * weight;
        }

        // 将计算出的最终总位移，写入对应的输出位置
        OutHighResOffsets[row] = accumulatedOffset;
    }

    return true;
}