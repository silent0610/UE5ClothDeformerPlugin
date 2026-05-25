#include "Misc/AutomationTest.h"
#include "SparseMappingMatrix.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatrixIdentityTest, "Cloth.SparseMatrix.Identity",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMatrixIdentityTest::RunTest(const FString& Parameters)
{
    FSparseMappingMatrix Matrix(2, 2);
    TArray<FTriplet> Triplets = {
        {0, 0, 1.0f},
        {1, 1, 1.0f}
    };
    Matrix.SetFromTriplet(Triplets);

    TArray<FVector> In = { FVector(1, 0, 0), FVector(0, 2, 0) };
    TArray<FVector> Out;
    Matrix.ApplyMapping(In, Out);

    TestEqual(TEXT("Row 0 X"), Out[0].X, 1.0);
    TestEqual(TEXT("Row 0 Y"), Out[0].Y, 0.0);
    TestEqual(TEXT("Row 1 X"), Out[1].X, 0.0);
    TestEqual(TEXT("Row 1 Y"), Out[1].Y, 2.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatrixInterpolationTest, "Cloth.SparseMatrix.Interpolation",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMatrixInterpolationTest::RunTest(const FString& Parameters)
{
    FSparseMappingMatrix Matrix(1, 2);
    TArray<FTriplet> Triplets = {
        {0, 0, 0.3f},
        {0, 1, 0.7f}
    };
    Matrix.SetFromTriplet(Triplets);

    TArray<FVector> In = { FVector(10, 0, 0), FVector(20, 0, 0) };
    TArray<FVector> Out;
    Matrix.ApplyMapping(In, Out);

    TestEqual(TEXT("Weighted X"), Out[0].X, 10.0f * 0.3f + 20.0f * 0.7f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatrixMismatchTest, "Cloth.SparseMatrix.Mismatch",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMatrixMismatchTest::RunTest(const FString& Parameters)
{
    FSparseMappingMatrix Matrix(2, 3);
    TArray<FTriplet> Triplets = {
        {0, 0, 1.0f},
        {1, 1, 1.0f}
    };
    Matrix.SetFromTriplet(Triplets);

    TArray<FVector> In = { FVector(1, 0, 0) };
    TArray<FVector> Out;
    bool bResult = Matrix.ApplyMapping(In, Out);

    TestFalse(TEXT("Returns false on size mismatch"), bResult);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMatrixEmptyTest, "Cloth.SparseMatrix.Empty",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMatrixEmptyTest::RunTest(const FString& Parameters)
{
    FSparseMappingMatrix Matrix(0, 0);
    TArray<FTriplet> Triplets;
    Matrix.SetFromTriplet(Triplets);

    TArray<FVector> In;
    TArray<FVector> Out;
    Matrix.ApplyMapping(In, Out);

    TestEqual(TEXT("Output empty for empty input"), Out.Num(), 0);
    return true;
}
