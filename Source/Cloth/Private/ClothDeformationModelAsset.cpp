// ClothDeformationModelAsset.cpp

#include "ClothDeformationModelAsset.h"
#if WITH_EDITOR
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

// 必须包含ONNX Runtime API才能解析模型。
#if PLATFORM_WINDOWS && PLATFORM_64BITS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

// Include the ONNX Runtime C++ API header.
#include "onnxruntime_cxx_api.h"

#if PLATFORM_WINDOWS && PLATFORM_64BITS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

void UClothDeformationModelAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

    // 如果file 被修改, 则需要重新加载模型
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UClothDeformationModelAsset, modelFile_))
    {
        // 1. Reset State
        inputNodeNames_.Empty();
        outputNodeNames_.Empty();
        modelData_.Empty();

        if (modelFile_.FilePath.IsEmpty())
        {
            return;
        }

        const FString AbsolutePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), modelFile_.FilePath);

        // 2. Load Data (Pure-ish)
        TArray<uint8> NewModelData = LoadModelBytes(AbsolutePath);
        if (NewModelData.Num() == 0)
        {
            return; 
        }

        // 3. Parse Metadata (Pure-ish)
        FOnnxMetadata Metadata = ParseModelMetadata(NewModelData);

        // 4. Update State (Single point of mutation)
        modelData_ = MoveTemp(NewModelData);
        if (Metadata.bIsValid)
        {
            inputNodeNames_ = Metadata.InputNames;
            outputNodeNames_ = Metadata.OutputNames;
        }
    }
}

TArray<uint8> UClothDeformationModelAsset::LoadModelBytes(const FString& FilePath)
{
    TArray<uint8> ResultData;
    if (!FFileHelper::LoadFileToArray(ResultData, *FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load ONNX model file: %s"), *FilePath);
        return TArray<uint8>();
    }
    UE_LOG(LogTemp, Log, TEXT("Loaded ONNX model data: %d bytes from %s"), ResultData.Num(), *FilePath);
    return ResultData;
}

UClothDeformationModelAsset::FOnnxMetadata UClothDeformationModelAsset::ParseModelMetadata(const TArray<uint8>& InModelData)
{
    FOnnxMetadata Metadata;
    if (InModelData.Num() == 0)
    {
        return Metadata;
    }

    try
    {
        // 创建临时环境和会话
        Ort::Env TempEnv(ORT_LOGGING_LEVEL_WARNING, "ModelAsset_Editor_Parse");
        Ort::SessionOptions SessionOptions;

        // 从内存创建 Session
        // 注意：Ort::Session 构造函数不接受 const void*，只接受 void* (虽然它可能只读)
        // 我们需要 const_cast，这是安全的，因为我们只读元数据
        void* DataPtr = const_cast<uint8*>(InModelData.GetData());
        Ort::Session TempSession(TempEnv, DataPtr, InModelData.Num(), SessionOptions);
        Ort::AllocatorWithDefaultOptions Allocator;

        // 输入节点
        for (size_t i = 0; i < TempSession.GetInputCount(); ++i)
        {
            char* TempInputName = TempSession.GetInputNameAllocated(i, Allocator).get();
            Metadata.InputNames.Add(FString(TempInputName));
        }

        // 输出节点
        for (size_t i = 0; i < TempSession.GetOutputCount(); ++i)
        {
            char* TempOutputName = TempSession.GetOutputNameAllocated(i, Allocator).get();
            Metadata.OutputNames.Add(FString(TempOutputName));
        }
        
        Metadata.bIsValid = true;
        UE_LOG(LogTemp, Log, TEXT("Successfully parsed metadata."));
    }
    catch (const Ort::Exception& e)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to parse ONNX metadata. ERROR: %hs"), e.what());
    }
    catch (const std::exception& e)
    {
         UE_LOG(LogTemp, Error, TEXT("Standard exception parsing ONNX metadata: %s"), UTF8_TO_TCHAR(e.what()));
    }

    return Metadata;
}

#endif // WITH_EDITOR