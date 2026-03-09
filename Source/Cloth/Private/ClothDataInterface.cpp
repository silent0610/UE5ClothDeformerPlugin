#include "ClothDataInterface.h"
#include "ClothDeformerComponent.h"
#include "ShaderParameterMetadataBuilder.h"
#include "OptimusDataDomain.h"

BEGIN_SHADER_PARAMETER_STRUCT(FClothDataInterfaceParameters, )
	// 顺序建议：基础类型在前，资源（SRV/UAV）在后，这是引擎的推荐对齐方式
	SHADER_PARAMETER(uint32, NumVertices)
	SHADER_PARAMETER_SRV(StructuredBuffer<float3>, ClothOffsets)
END_SHADER_PARAMETER_STRUCT()
 
class FClothDataProviderProxy : public FComputeDataProviderRenderProxy
{
public:
	// 增加 NumVertices 参数
	FClothDataProviderProxy(FShaderResourceViewRHIRef InOffsetSRV, uint32 InNumVertices)
		: OffsetSRV(InOffsetSRV), NumVertices(InNumVertices)
	{
	}

	virtual void GatherDispatchData(FDispatchData const& InDispatchData) override
	{
		if (OffsetSRV.IsValid())
		{
			// 【核心改动】使用模板视图自动填充数据
			auto ParameterArray = MakeStridedParameterView<FClothDataInterfaceParameters>(InDispatchData);
			for (int32 i = 0; i < ParameterArray.Num(); ++i)
			{
				ParameterArray[i].NumVertices = NumVertices;
				ParameterArray[i].ClothOffsets = OffsetSRV;
			}
		}
	}
private:
	FShaderResourceViewRHIRef OffsetSRV;
	uint32 NumVertices;
};

// =====================================================================
// UClothDataProvider 实现
// =====================================================================

FComputeDataProviderRenderProxy* UClothDataProvider::GetRenderProxy()
{
	FShaderResourceViewRHIRef SRV = nullptr;
	uint32 NumVertices = 0;
	if (ClothComponent)
	{
		SRV = ClothComponent->GetOffsetBufferSRV();
		NumVertices = ClothComponent->GetVertexCount(); // 假设你组件里有这个 Getter
	}
	return new FClothDataProviderProxy(SRV, NumVertices);
}
// =====================================================================
// UClothDataInterface 实现
// =====================================================================

FString UClothDataInterface::GetDisplayName() const
{
	return TEXT("Snug Cloth Offsets");
}

TArray<FOptimusCDIPinDefinition> UClothDataInterface::GetPinDefinitions() const
{
	TArray<FOptimusCDIPinDefinition> Pins;

	// 使用单级上下文查找的构造函数
	// 参数：引脚名, 读取函数名, 上下文域(Vertex), 计数函数名
	Pins.Add(FOptimusCDIPinDefinition(
		TEXT("Offset"),
		TEXT("ReadClothOffset"),
		Optimus::DomainName::Vertex,
		TEXT("ReadVertexCount")
	));

	return Pins;
}
TSubclassOf<UActorComponent> UClothDataInterface::GetRequiredComponentClass() const
{
	return UClothDeformerComponent::StaticClass();
	//return nullptr;
}

void UClothDataInterface::GetSupportedInputs(TArray<FShaderFunctionDefinition>& OutFunctions) const
{
	// 1. 声明：读取顶点总数的函数
	OutFunctions.AddDefaulted_GetRef()
		.SetName(TEXT("ReadVertexCount")) // 对应我们之前定义的计数函数名
		.AddReturnType(EShaderFundamentalType::Uint);

	// 2. 声明：读取 AI 布料偏移的函数
	OutFunctions.AddDefaulted_GetRef()
		.SetName(TEXT("ReadClothOffset")) // 对应我们引脚定义里的函数名
		.AddReturnType(EShaderFundamentalType::Float, 3) // 返回偏移向量 float3
		.AddParam(EShaderFundamentalType::Uint); // 输入参数是顶点索引 VertexIndex
}

void UClothDataInterface::GetShaderParameters(TCHAR const* UID, FShaderParametersMetadataBuilder& InOutBuilder, FShaderParametersMetadataAllocations& InOutAllocations) const
{
	// 这一行代码会自动生成 UID_NumVertices 和 UID_ClothOffsets 两个 GPU 变量
	InOutBuilder.AddNestedStruct<FClothDataInterfaceParameters>(UID);
}

const TCHAR* UClothDataInterface::GetShaderVirtualPath() const
{
	return TemplateFilePath;
}
void UClothDataInterface::GetShaderHash(FString& InOutKey) const
{
	GetShaderFileHash(TemplateFilePath, EShaderPlatform::SP_PCD3D_SM6).AppendString(InOutKey);
}
void UClothDataInterface::GetHLSL(FString& OutHLSL, FString const& InDataInterfaceName) const
{
	// 1. 定义替换参数表
	TMap<FString, FStringFormatArg> TemplateArgs = {
		{ TEXT("DataInterfaceName"), InDataInterfaceName },
	};

	// 2. 从虚拟路径加载文件内容
	FString TemplateFile;
	if (LoadShaderSourceFile(TemplateFilePath, EShaderPlatform::SP_PCD3D_SM6, &TemplateFile, nullptr))
	{
		// 3. 执行格式化替换并追加到输出
		OutHLSL += FString::Format(*TemplateFile, TemplateArgs);
	}
}

UComputeDataProvider* UClothDataInterface::CreateDataProvider(TObjectPtr<UObject> InBinding, uint64 InInputMask, uint64 InOutputMask) const
{
	UClothDataProvider* Provider = NewObject<UClothDataProvider>();

	// 自动寻找挂载在同一个 Actor 上的 AI 组件
	if (AActor* Actor = Cast<AActor>(InBinding))
	{
		Provider->ClothComponent = Actor->FindComponentByClass<UClothDeformerComponent>();
	}
	else if (UActorComponent* Component = Cast<UActorComponent>(InBinding))
	{
		Provider->ClothComponent = Component->GetOwner()->FindComponentByClass<UClothDeformerComponent>();
	}

	return Provider;
}
