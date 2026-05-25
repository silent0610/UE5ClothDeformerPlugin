#include "Misc/AutomationTest.h"
#include "OnnxModelInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOnnxNullAssetTest, "Cloth.OnnxInstance.NullAsset",
    EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FOnnxNullAssetTest::RunTest(const FString& Parameters)
{
    FOnnxModelInstance Instance(nullptr);
    TestFalse(TEXT("Null asset: not initialized"), Instance.IsInitialized());

    TArray<float> In, Out;
    TestFalse(TEXT("Null asset: Run returns false"), Instance.Run(In, Out));
    return true;
}
