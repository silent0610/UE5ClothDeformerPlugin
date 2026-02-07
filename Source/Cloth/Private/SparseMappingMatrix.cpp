#include "SparseMappingMatrix.h"

FTriplet::FTriplet(int32 InRow, int32 InCol, float InValue) : Row(InRow), Col(InCol), Value(InValue) { ; }

FSparseMappingMatrix::FSparseMappingMatrix(int32 InRow, int32 InCol) : NumRow(InRow), NumCol(InCol)
{
    ;
}
void FSparseMappingMatrix::SetFromTriplet(const TArray<FTriplet> &Triplets)
{
    if (NumRow <= 0)
    {
        return;
    }

    const int32 NumNonZero{Triplets.Num()};

    // 计数
    TArray<int32> RowCount{};
    RowCount.AddZeroed(NumRow);
    for (int32 k{0}; k< NumNonZero; ++k)
    {
        if (Triplets[k].Row >= 0 && Triplets[k].Row < NumRow)
        {
            RowCount[Triplets[k].Row] += 1;
        }
    }

    RowPtr.Empty(NumRow + 1);
    RowPtr.Add(0);
    int32 CumulativeCount{0};
    for (int32 i{0}; i < NumRow; ++i)
    {
        CumulativeCount += RowCount[i];
        RowPtr.Add(CumulativeCount);
    }

    Value.Empty(NumNonZero);
	Value.AddUninitialized(NumNonZero);
    ColIndice.Empty(NumNonZero);
	ColIndice.AddUninitialized(NumNonZero);

    TArray<int32> WritePos{ RowPtr };
    for (int32 k{ 0 }; k < NumNonZero; ++k)
    {
        const FTriplet &Triplet = Triplets[k];
        const int32 Row = Triplet.Row;
        const int32 DestPos = WritePos[Row];
        ColIndice[DestPos] = Triplet.Col;
        Value[DestPos] = Triplet.Value;
		WritePos[Row] += 1;
    }
}

// TODO  修改为从低模映射到低模
bool FSparseMappingMatrix::ApplyMapping(const TArray<FVector>& InLowResOffsets, TArray<FVector>& OutHighResOffsets) const
{
    //if (InHighResVertices.Num() != NumCol)
    //{
    //    UE_LOG(LogTemp, Error, TEXT("ApplyMapping: Input vertex count (%d) mismatch. Matrix requires %d."), InHighResVertices.Num(), NumCol);
    //    return false;
    //}

    //OutLowResVertices.SetNumUninitialized(NumRow);

    //// TODO 并行
    //for (int32 i = 0; i < NumRow; ++i) // 遍历低模的每一个顶点 (矩阵的每一行)
    //{
    //    const int32 RowStart = RowPtr[i];     // 第 i 行的起始索引
    //    const int32 RowEnd = RowPtr[i + 1];   // 第 i 行的结束索引

    //    FVector3f NewLowResPos = FVector3f::ZeroVector;

    //    // 遍历第 i 行的所有非零元素 (M_ij)
    //    for (int32 k = RowStart; k < RowEnd; ++k)
    //    {
    //        const float Weight = Value[k];
    //        const int32 HighResIndex = ColIndice[k];

    //        // 增加鲁棒性：确保高模索引不会越界
    //        if (HighResIndex >= 0 && HighResIndex < InHighResVertices.Num())
    //        {
    //            NewLowResPos += InHighResVertices[HighResIndex] * Weight;
    //        }
    //    }
    //    OutLowResVertices[i] = NewLowResPos;
    //}
    //return true;
}