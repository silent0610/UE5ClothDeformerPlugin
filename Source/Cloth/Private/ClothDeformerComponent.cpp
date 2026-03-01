#include "ClothDeformerComponent.h"
#include "OnnxModelInstance.h"
#include "MeshMappingAsset.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Components/DynamicMeshComponent.h"


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
bool UClothDeformerComponent::RunInference(const TMap<FString, TArray<float>>& InputData, TArray<float>& HiddenState, TMap<FString, TArray<float>>& OutputData)
{
    if (!IsInitialized())
    {
        UE_LOG(LogTemp, Warning, TEXT("RunInference called but component is not initialized"));
        return false;
    }

    if (!modelInstance_)
        return false;
    return modelInstance_->Run(InputData,HiddenState, OutputData);
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

	// 这部分是测试代码，确保推理流程在Tick中正常工作。
    //// 确保已初始化且有测试数据
    //if (IsInitialized() && TestInputArray.Num() > 0)
    //{
    //    if (RunInference(TestInputArray, TestOutputArray))
    //    {
    //        // For debugging, log the first element of the output array
    //        if (TestOutputArray.Num() > 0)
    //        {
    //            UE_LOG(LogTemp, Log, TEXT("Inference successful. First output element: %f"), TestOutputArray[0]);
    //        }
    //    }
    //    else
    //    {
    //        UE_LOG(LogTemp, Warning, TEXT("RunInference failed in TickComponent."));
    //    }
    //}

    // 1. 获取输入
    TMap<FString, TArray<float>> ModelInputs = InputAdapter->ExtractInputs(DeltaTime);


    // 2. 运行推理 (拿到低模偏移)
    TMap<FString, TArray<float>> ModelOutputs; // 也可以是称为 低模映射
    if (!RunInference(ModelInputs, CurrentHiddenState, ModelOutputs))
    {
        // 推理失败，保持当前姿态，直接退出
        return;
    }

    TArray<float> lowResRawOffsets;

    for (const auto& pair : ModelOutputs)
    {
        lowResRawOffsets = pair.Value;
        break;
    }

    int32 numLowResVerts = lowResRawOffsets.Num() / 3;
    TArray<FVector> lowResVectors;
    lowResVectors.SetNumUninitialized(numLowResVerts);

    for (int32 i = 0; i < numLowResVerts; ++i)
    {
        // 可能需要在这里做坐标系转换
        lowResVectors[i] = FVector(
            lowResRawOffsets[i * 3 + 0], // X
            lowResRawOffsets[i * 3 + 1], // Y
            lowResRawOffsets[i * 3 + 2]  // Z
        );
    }

    // 3. 映射到高模
    TArray<FVector> HighResOffsets;
    
    if (!MappingAsset->MappingData.ApplyMapping(lowResVectors,HighResOffsets))
    {
        UE_LOG(LogTemp, Error, TEXT("Tick: ApplyMapping 失败"));
        return;
    }
    
    USkeletalMeshComponent* targetMesh = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
    if (targetMesh)
    {
        UpdateMesh(targetMesh, HighResOffsets);
    }
}

void UClothDeformerComponent::UpdateMesh(USkeletalMeshComponent* targetMesh, const TArray<FVector>& HighResOffsets)
{
    // 感觉有点难写, 先留空
    ;
}
