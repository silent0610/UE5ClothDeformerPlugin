#include "ClothDeformerComponent.h"
#include "OnnxModelInstance.h"
#include "MeshMappingAsset.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformFilemanager.h"

// 包含ONNX Runtime头文件用于测试
#if PLATFORM_WINDOWS && PLATFORM_64BITS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <psapi.h> // 用于检测加载的模块
#endif
#include "onnxruntime_cxx_api.h"
#if PLATFORM_WINDOWS && PLATFORM_64BITS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

// The UClothDeformerComponent must define its destructor in the .cpp file.
// This is required because the header file only has a forward declaration of FOnnxModelInstance.
// The TUniquePtr<FOnnxModelInstance> member needs the full definition of FOnnxModelInstance to be able to call its destructor.
// By defining the UClothDeformerComponent destructor here, we ensure that the compiler sees the full definition of FOnnxModelInstance
// (included from OnnxModelInstance.h) before it tries to generate the code to destroy the modelInstance_ member.
UClothDeformerComponent::~UClothDeformerComponent()
{
    // Intentionally empty. The key is that this definition exists here in the .cpp file.
}

UClothDeformerComponent::UClothDeformerComponent()
{
    // 启用Tick，以便每帧运行
    PrimaryComponentTick.bCanEverTick = true;
}
void UClothDeformerComponent::BeginPlay()
{
    Super::BeginPlay();

    // 尝试初始化ONNX模型
    UE_LOG(LogTemp, Log, TEXT("Cloth Deformer Component BeginPlay - attempting to initialize model..."));

    if (Initialize())
    {
        UE_LOG(LogTemp, Log, TEXT("Cloth Deformer Component initialized successfully"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Cloth Deformer Component initialization failed - no model asset specified"));
    }
}
void UClothDeformerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // 清理资源
    modelInstance_.Reset();

    Super::EndPlay(EndPlayReason);
}
bool UClothDeformerComponent::Initialize()
{
    try
    {
        if (modelAsset_)
        {
            modelInstance_ = MakeUnique<FOnnxModelInstance>(modelAsset_);
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Cloth Deformer Component initialization failed - no model asset specified"));
            modelInstance_.Reset();
            bIsInitialized = false;
            return false;
        }

        if (modelInstance_ && modelInstance_->IsInitialized())
        {
            UE_LOG(LogTemp, Log, TEXT("ONNX Model Instance created successfully"));
            bIsInitialized = true;
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to initialize ONNX Model Instance"));
            modelInstance_.Reset();
            bIsInitialized = false;
            return false;
        }
    }
    catch (const std::exception &e)
    {
        UE_LOG(LogTemp, Error, TEXT("Exception creating ONNX Model Instance: %s"), UTF8_TO_TCHAR(e.what()));
        modelInstance_.Reset();
        bIsInitialized = false;
        return false;
    }
}

bool UClothDeformerComponent::RunInference(const TArray<float> &InputData, TArray<float> &OutputData)
{
    if (!IsInitialized())
    {
        UE_LOG(LogTemp, Warning, TEXT("RunInference called but component is not initialized"));
        return false;
    }

    if (!modelInstance_)
        return false;
    return modelInstance_->Run(InputData, OutputData);
}

bool UClothDeformerComponent::IsInitialized() const
{
    return bIsInitialized && modelInstance_ && modelInstance_->IsInitialized();
}

void UClothDeformerComponent::Reset()
{
    modelInstance_.Reset();
    bIsInitialized = false;
    UE_LOG(LogTemp, Log, TEXT("ONNX Component reset"));
}
void UClothDeformerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // 确保已初始化且有测试数据
    if (IsInitialized() && TestInputArray.Num() > 0)
    {
        if (RunInference(TestInputArray, TestOutputArray))
        {
            // For debugging, log the first element of the output array
            if (TestOutputArray.Num() > 0)
            {
                UE_LOG(LogTemp, Log, TEXT("Inference successful. First output element: %f"), TestOutputArray[0]);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("RunInference failed in TickComponent."));
        }
    }
    //// 1. 获取输入
    //TArray<float> ModelInputs;
    //InputAdapter->ExtractInputs(ModelInputs);

    //// 2. 运行推理 (拿到低模偏移)
    //TArray<float> LowResOffsetsRaw;
    //modelInstance_->Run(ModelInputs, CurrentHiddenState, LowResOffsetsRaw);

    //// 3. 映射到高模
    //TArray<FVector> HighResOffsets;
    //// (此处需将 LowResOffsetsRaw 转换为 FVector 数组)
    //MappingAsset->MappingData.ApplyMapping(LowResLowResVectors, HighResOffsets);

    //// 4. [新增工作] 直接在这里应用到网格
    //UDynamicMeshComponent* TargetMesh = GetOwner()->FindComponentByClass<UDynamicMeshComponent>();
    //if (TargetMesh)
    //{
    //    // 假设使用 DynamicMesh，直接更新顶点
    //    // (伪代码：将 HighResOffsets 加到 BasePose 上并通知更新)
    //    UpdateDynamicMesh(TargetMesh, HighResOffsets);
    //}

}
