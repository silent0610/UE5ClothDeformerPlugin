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

    // 1. 获取变更属性的名称
    FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
    FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

    // 2. 打印调试信息 (看看 UE 到底传回了什么)
    UE_LOG(LogTemp, Warning, TEXT("PostEditChange: Property=[%s], MemberProperty=[%s]"), *PropertyName.ToString(), *MemberPropertyName.ToString());

    // 3. 关键修正：检查 MemberProperty (成员变量名)，而不是 Property (内部属性名)
    if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UClothDeformationModelAsset, modelFile_))
    {
        UE_LOG(LogTemp, Log, TEXT("Detected model file change! Reloading..."));

        // === 下面是你的原有逻辑 ===

        // 1. Reset State
        inputNodeNames_.Empty();
        outputNodeNames_.Empty();
        modelData_.Empty();

        if (modelFile_.FilePath.IsEmpty())
        {
            return;
        }

        // 处理相对路径问题 (增加 FPaths::ProjectDir)
        FString AbsolutePath = modelFile_.FilePath;
        if (FPaths::IsRelative(AbsolutePath))
        {
            AbsolutePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), AbsolutePath);
        }

        // 2. Load Data
        TArray<uint8> NewModelData = LoadModelBytes(AbsolutePath);
        if (NewModelData.Num() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to load bytes from: %s"), *AbsolutePath);
            return;
        }

        // 3. Parse Metadata
        FOnnxMetadata Metadata = ParseModelMetadata(NewModelData);

        // 4. Update State
        modelData_ = MoveTemp(NewModelData);
        if (Metadata.bIsValid)
        {
            inputNodeNames_ = Metadata.InputNames;
            outputNodeNames_ = Metadata.OutputNames;
            UE_LOG(LogTemp, Log, TEXT("Metadata parsed. Inputs: %d, Outputs: %d"), inputNodeNames_.Num(), outputNodeNames_.Num());
        }

        // 标记资产已修改 (Dirty)，这样左上角会有小星号，提示保存
        MarkPackageDirty();
    }
}
void UClothDeformationModelAsset::LoadModelData()
{
    // 防止误点，做个简单的日志开始
    UE_LOG(LogTemp, Warning, TEXT("LoadModelData Button Clicked!"));

    if (modelFile_.FilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("File path is empty!"));
        return;
    }

    // 路径处理
    FString AbsolutePath = modelFile_.FilePath;
    if (FPaths::IsRelative(AbsolutePath))
    {
        AbsolutePath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), AbsolutePath);
    }

    // 读取文件
    TArray<uint8> TempData;
    if (FFileHelper::LoadFileToArray(TempData, *AbsolutePath))
    {
        // 只有读取成功才覆盖
        modelData_ = MoveTemp(TempData);

        UE_LOG(LogTemp, Log, TEXT("SUCCESS: Loaded %d bytes from %s"), modelData_.Num(), *AbsolutePath);

        // 解析元数据 (假设你有这个函数)
        // ParseModelMetadata(modelData_);

        // 标记为脏，确保保存
        MarkPackageDirty();
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("FAILED to read file: %s"), *AbsolutePath);
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