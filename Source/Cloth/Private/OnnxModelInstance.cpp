#include "OnnxModelInstance.h"
#include "ClothDeformationModelAsset.h"

// 包含ONNX Runtime的实现头文件
#if PLATFORM_WINDOWS && PLATFORM_64BITS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif
#include "onnxruntime_cxx_api.h"
#if PLATFORM_WINDOWS && PLATFORM_64BITS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

FOnnxModelInstance::FOnnxModelInstance(UClothDeformationModelAsset *InModelAsset)
{
    UE_LOG(LogTemp, Log, TEXT("Creating FOnnxModelInstance..."));

    if (!InModelAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid Model Asset passed to FOnnxModelInstance"));
        return;
    }

    // 检查是否有模型数据
    if (InModelAsset->modelData_.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Model Asset has no data (modelData_ is empty). Make sure to re-import or update the asset."));
        return;
    }

    try
    {
        // 创建ONNX环境
        env_ = MakeUnique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "ClothModel");

        // 创建会话选项
        Ort::SessionOptions sessionOptions;
        sessionOptions.SetIntraOpNumThreads(1);
        sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

        UE_LOG(LogTemp, Log, TEXT("Creating ONNX Session from memory (%d bytes)..."), InModelAsset->modelData_.Num());

        // 从内存创建会话
        session_ = MakeUnique<Ort::Session>(*env_, InModelAsset->modelData_.GetData(), InModelAsset->modelData_.Num(), sessionOptions);

        UE_LOG(LogTemp, Log, TEXT("ONNX Session created successfully"));

        // 获取模型输入输出信息
        Ort::AllocatorWithDefaultOptions allocator;

        // Input Info
        size_t numInputNodes = session_->GetInputCount();
        if (numInputNodes > 0)
        {
            auto inputName = session_->GetInputNameAllocated(0, allocator);
            inputNodeName_ = FString(UTF8_TO_TCHAR(inputName.get()));

            Ort::TypeInfo typeInfo = session_->GetInputTypeInfo(0);
            auto tensorInfo = typeInfo.GetTensorTypeAndShapeInfo();
            inputNodeDims_ = tensorInfo.GetShape();

            UE_LOG(LogTemp, Log, TEXT("Input Node: %s, Dimensions: %d"), *inputNodeName_, inputNodeDims_.Num());
        }

        // Output Info
        size_t numOutputNodes = session_->GetOutputCount();
        if (numOutputNodes > 0)
        {
            auto outputName = session_->GetOutputNameAllocated(0, allocator);
            outputNodeName_ = FString(UTF8_TO_TCHAR(outputName.get()));

            // 可以存储 outputNodeDims_ 如果需要
            UE_LOG(LogTemp, Log, TEXT("Output Node: %s"), *outputNodeName_);
        }

        bIsInitialized_ = true;
        UE_LOG(LogTemp, Log, TEXT("FOnnxModelInstance initialized successfully from asset memory"));
    }
    catch (const Ort::Exception &e)
    {
        UE_LOG(LogTemp, Error, TEXT("ONNX Runtime error in constructor: %s"), UTF8_TO_TCHAR(e.what()));
        bIsInitialized_ = false;
    }
    catch (const std::exception &e)
    {
        UE_LOG(LogTemp, Error, TEXT("Standard exception in constructor: %s"), UTF8_TO_TCHAR(e.what()));
        bIsInitialized_ = false;
    }
}

FOnnxModelInstance::~FOnnxModelInstance()
{
}

bool FOnnxModelInstance::IsInitialized() const
{
    return bIsInitialized_;
}

bool FOnnxModelInstance::Run(const TArray<float> &InputData, TArray<float> &OutputData)
{
    if (!bIsInitialized_ || !session_)
    {
        UE_LOG(LogTemp, Error, TEXT("Run: Model not initialized."));
        return false;
    }

    try
    {
        // 1. 计算输入张量维度
        std::vector<int64_t> actualInputDims;

        // Warn: 每次重新进行检查, 如果模型确定所有维度都是静态的，并且每次推理的输入数据大小都固定不变, 可以缓存
        if (!CalculateInputTensorDimensions(InputData.Num(), actualInputDims))
        {
            return false;
        }

        // 2. 创建 ONNX 输入张量
        Ort::Value inputTensor{nullptr}; // Initialize with nullptr
        if (!CreateInputTensor(InputData, actualInputDims, inputTensor))
        {
            return false;
        }

        // 3. 准备节点名称
        Ort::AllocatorWithDefaultOptions allocator;
        auto inputNamePtr = session_->GetInputNameAllocated(0, allocator);
        auto outputNamePtr = session_->GetOutputNameAllocated(0, allocator);

        const char *inputNames[] = {inputNamePtr.get()};
        const char *outputNames[] = {outputNamePtr.get()};

        // 4. 执行推理
        auto outputTensors = session_->Run(
            Ort::RunOptions{nullptr},
            inputNames,
            &inputTensor,
            1, // num inputs
            outputNames,
            1 // num outputs
        );

        // 5. 提取输出
        if (outputTensors.size() > 0 && outputTensors[0].IsTensor())
        {
            return ExtractOutputTensorData(outputTensors[0], OutputData);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Run: Inference returned no valid tensors."));
            return false;
        }
    }
    catch (const Ort::Exception &e)
    {
        UE_LOG(LogTemp, Error, TEXT("Run: ONNX Runtime error: %s"), UTF8_TO_TCHAR(e.what()));
    }
    catch (const std::exception &e)
    {
        UE_LOG(LogTemp, Error, TEXT("Run: Standard exception: %s"), UTF8_TO_TCHAR(e.what()));
    }

    return false;
}

bool FOnnxModelInstance::CalculateInputTensorDimensions(int32 InInputDataSize, std::vector<int64_t> &OutActualDims) const
{
    OutActualDims = inputNodeDims_; // Start with the model's expected dimensions

    int64_t staticElementCount = 1;
    int dynamicDimIndex = -1; // Warn: 在大多数常见的深度学习模型中，通常只有一个动态维度, 如果需要支持多个动态维度, 需要修改

    // Analyze dimensions
    for (size_t i = 0; i < OutActualDims.size(); ++i)
    {
        if (OutActualDims[i] == -1)
        {
            dynamicDimIndex = (int)i;
        }
        else
        {
            staticElementCount *= OutActualDims[i];
        }
    }

    // Infer dynamic dimension if necessary
    if (dynamicDimIndex != -1)
    {
        if (staticElementCount > 0 && InInputDataSize % staticElementCount == 0)
        {
            OutActualDims[dynamicDimIndex] = InInputDataSize / staticElementCount;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("CalculateInputTensorDimensions: Input data size %d is incompatible with model static structure (element count: %lld)."), InInputDataSize, staticElementCount);
            return false;
        }
    }
    else
    {
        // Strict check for purely static models
        if (InInputDataSize != staticElementCount)
        {
            UE_LOG(LogTemp, Warning, TEXT("CalculateInputTensorDimensions: Input data size %d does not match expected model size %lld."), InInputDataSize, staticElementCount);
            return false;
        }
    }
    return true;
}

bool FOnnxModelInstance::CreateInputTensor(const TArray<float> &InInputData, const std::vector<int64_t> &InActualDims, Ort::Value &OutInputTensor) const
{
    Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault); // Warn:从 Mesh 获取顶点数据通常是在 CPU 上完成的. 理论上可以将顶点数据存在GPU上进行加速
    try
    {
        OutInputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo,
            const_cast<float *>(InInputData.GetData()),
            InInputData.Num(),
            InActualDims.data(),
            InActualDims.size());
    }
    catch (const Ort::Exception &e)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateInputTensor: ONNX Runtime error creating tensor: %s"), UTF8_TO_TCHAR(e.what()));
        return false;
    }
    catch (const std::exception &e)
    {
        UE_LOG(LogTemp, Error, TEXT("CreateInputTensor: Standard exception creating tensor: %s"), UTF8_TO_TCHAR(e.what()));
        return false;
    }
    return true;
}

bool FOnnxModelInstance::ExtractOutputTensorData(Ort::Value &InOutputTensor, TArray<float> &OutOutputData) const
{
    if (!InOutputTensor.IsTensor())
    {
        UE_LOG(LogTemp, Error, TEXT("ExtractOutputTensorData: Provided Ort::Value is not a tensor."));
        return false;
    }

    const float *floatArr = InOutputTensor.GetTensorData<float>();
    size_t outputElementCount = InOutputTensor.GetTensorTypeAndShapeInfo().GetElementCount();

    OutOutputData.SetNumUninitialized(outputElementCount);
    FMemory::Memcpy(OutOutputData.GetData(), floatArr, outputElementCount * sizeof(float)); // Warn: 当前额外进行了一次拷贝, 理论上可以通过 传入一个预先分配好的 Ort::Value 作为输出

    return true;
}