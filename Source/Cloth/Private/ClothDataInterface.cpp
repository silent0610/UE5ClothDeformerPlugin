#include "ClothDataInterface.h"
#include "ClothDeformerComponent.h"
#include "ShaderParameterMetadataBuilder.h"
#include "OptimusDataDomain.h"

class FClothDataProviderProxy : public FComputeDataProviderRenderProxy
{
public:
	FClothDataProviderProxy(FShaderResourceViewRHIRef InOffsetSRV)
		: OffsetSRV(InOffsetSRV)
	{
	}
	virtual void GatherDispatchData(FDispatchData const& InDispatchData) 
	{
		if (OffsetSRV.IsValid() && InDispatchData.ParameterBuffer != nullptr)	
		{// 1. 找到属于我们这个节点的内存起始位置 (Base + Offset)
			uint8* MyMemoryStart = InDispatchData.ParameterBuffer + InDispatchData.ParameterBufferOffset;

			// 2. 在这块专属内存中写入 SRV 裸指针
			FRHIShaderResourceView** SRVMemorySlot = (FRHIShaderResourceView**)MyMemoryStart;
			*SRVMemorySlot = OffsetSRV.GetReference();
		}
	}
private:
	FShaderResourceViewRHIRef OffsetSRV;
};

// =====================================================================
// UClothDataProvider 实现
// =====================================================================

FComputeDataProviderRenderProxy* UClothDataProvider::GetRenderProxy()
{
	FShaderResourceViewRHIRef SRV = nullptr;
	if (ClothComponent)
	{
		// 调用你组件中的 Getter
		SRV = ClothComponent->GetOffsetBufferSRV();
	}
	return new FClothDataProviderProxy(SRV);
}

// =====================================================================
// UClothDataInterface 实现
// =====================================================================

FString UClothDataInterface::GetDisplayName() const
{
	return TEXT("AI Cloth Offsets");
}

TArray<FOptimusCDIPinDefinition> UClothDataInterface::GetPinDefinitions() const
{
	TArray<FOptimusCDIPinDefinition> Pins;

	// 使用单级上下文查找的构造函数
	// 参数：引脚名, 读取函数名, 上下文域(Vertex), 计数函数名
	Pins.Add(FOptimusCDIPinDefinition(
		TEXT("Offset"),
		TEXT("ReadClothOffset"),
		FName(TEXT("Vertex")),
		TEXT("ReadVertexCount")
	));

	return Pins;
}
TSubclassOf<UActorComponent> UClothDataInterface::GetRequiredComponentClass() const
{
	return UClothDeformerComponent::StaticClass();
}

void UClothDataInterface::GetSupportedInputs(TArray<FShaderFunctionDefinition>& OutFunctions) const
{
	// 1. 原本的读取偏移函数 (带一个 uint VertexIndex 输入)
	FShaderFunctionDefinition FnRead;
	FnRead.Name = TEXT("ReadClothOffset");
	FnRead.bHasReturnType = true;

	FShaderParamTypeDefinition ReturnParam;
	ReturnParam.TypeDeclaration = TEXT("float3");
	ReturnParam.Name = TEXT("ReturnValue");
	FnRead.ParamTypes.Add(ReturnParam);

	FShaderParamTypeDefinition IndexParam;
	IndexParam.TypeDeclaration = TEXT("uint");
	IndexParam.Name = TEXT("VertexIndex");
	FnRead.ParamTypes.Add(IndexParam);

	OutFunctions.Add(FnRead);

	// 2. 【新增】获取顶点总数的函数 (无输入，返回 uint)
	FShaderFunctionDefinition FnCount;
	FnCount.Name = TEXT("ReadVertexCount");
	FnCount.bHasReturnType = true;

	FShaderParamTypeDefinition CountReturnParam;
	CountReturnParam.TypeDeclaration = TEXT("uint");
	CountReturnParam.Name = TEXT("ReturnValue");
	FnCount.ParamTypes.Add(CountReturnParam);

	OutFunctions.Add(FnCount);
}

void UClothDataInterface::GetHLSL(FString& OutHLSL, FString const& InDataInterfaceName) const
{
	// 1. 声明带 UID 前缀的 Buffer
	OutHLSL += FString::Printf(TEXT("StructuredBuffer<float3> %s_ClothOffsets;\n"), *InDataInterfaceName);

	// 2. 【新增】实现顶点计数函数
	OutHLSL += FString::Printf(TEXT("uint %s_ReadVertexCount()\n"), *InDataInterfaceName);
	OutHLSL += TEXT("{\n");
	OutHLSL += TEXT("    uint NumStructs, Stride;\n");
	// 直接获取 Buffer 的大小，极致高效
	OutHLSL += FString::Printf(TEXT("    %s_ClothOffsets.GetDimensions(NumStructs, Stride);\n"), *InDataInterfaceName);
	OutHLSL += TEXT("    return NumStructs;\n");
	OutHLSL += TEXT("}\n");

	// 3. 实现读取偏移函数
	OutHLSL += FString::Printf(TEXT("float3 %s_ReadClothOffset(uint VertexIndex)\n"), *InDataInterfaceName);
	OutHLSL += TEXT("{\n");
	OutHLSL += FString::Printf(TEXT("    return %s_ClothOffsets[VertexIndex];\n"), *InDataInterfaceName);
	OutHLSL += TEXT("}\n");
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
void UClothDataInterface::GetShaderParameters(TCHAR const* UID, FShaderParametersMetadataBuilder& InOutBuilder, FShaderParametersMetadataAllocations& InOutAllocations) const
{
	// 告诉引擎：在这块内存里预留一个 SRV 的位置，对应 HLSL 里的 ClothOffsets
	FString BufferName = FString::Printf(TEXT("%s_ClothOffsets"), UID);

	InOutBuilder.AddBufferSRV(*BufferName, TEXT("StructuredBuffer<float3>"));
}