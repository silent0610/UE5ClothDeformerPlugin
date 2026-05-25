#include "Misc/AutomationTest.h"
#include "SnugInputAdapter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdapterCreationTest, "Cloth.Adapter.Creation",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAdapterCreationTest::RunTest(const FString& Parameters)
{
    FSnugInputAdapter Adapter;
    TestTrue(TEXT("Adapter created"), true);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAdapterNoMeshReturnsEmpty, "Cloth.Adapter.EmptyInputsWithoutMesh",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FAdapterNoMeshReturnsEmpty::RunTest(const FString& Parameters)
{
    FSnugInputAdapter Adapter;
    TMap<FString, TArray<float>> Inputs = Adapter.ExtractInputs(0.016f);
    TestEqual(TEXT("Returns empty map without SkelComp"), Inputs.Num(), 0);
    return true;
}
