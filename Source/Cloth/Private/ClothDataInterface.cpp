#include "ClothDataInterface.h"
#include "ClothDeformerComponent.h"
#include "ShaderParameterMetadataBuilder.h"


class FClothDataProviderProxy : public FComputeDataProviderRenderProxy
{
public:
	FClothDataProviderProxy(FShaderResourceViewRHIRef InOffsetSRV)
		: OffsetSRV(InOffsetSRV)
	{
	}
	virtual void GatherDispatchData(FDispatchSetup const& InDispatchSetup, FCollectedDispatchData& InOutDispatchData) override
	{
		;
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
	// 定义输出引脚 "Offset"，关联到 HLSL 的 ReadData 操作
	Pins.Add({ TEXT("Offset"), "ReadData" });
	return Pins;
}

TSubclassOf<UActorComponent> UClothDataInterface::GetRequiredComponentClass() const
{
	return UClothDeformerComponent::StaticClass();
}

void UClothDataInterface::GetSupportedInputs(TArray<FShaderFunctionDefinition>& OutFunctions) const
{
	FShaderFunctionDefinition Fn;
	Fn.Name = TEXT("ReadClothOffset");
	Fn.bHasReturnType = true;

	// 1. 在 UE5.6 中，如果 bHasReturnType 为 true，
	// ParamTypes 数组的第一个元素通常作为返回值定义。
	FShaderParamTypeDefinition ReturnParam;
	ReturnParam.TypeDeclaration = TEXT("float3");
	ReturnParam.Name = TEXT("ReturnValue"); // 名字可以随意，HLSL 仅将其作为返回值类型
	Fn.ParamTypes.Add(ReturnParam);

	// 2. 第二个元素开始，才是真正的输入参数
	FShaderParamTypeDefinition IndexParam;
	IndexParam.TypeDeclaration = TEXT("uint");
	IndexParam.Name = TEXT("VertexIndex");  // 参数名
	Fn.ParamTypes.Add(IndexParam);

	OutFunctions.Add(Fn);
}

void UClothDataInterface::GetHLSL(FString& OutHLSL, FString const& InDataInterfaceName) const
{
	// 注入 Shader 变量声明
	OutHLSL += TEXT("StructuredBuffer<float3> ClothOffsets;\n");

	// 注入读取函数实现
	OutHLSL += TEXT("float3 ReadClothOffset(uint VertexIndex)\n");
	OutHLSL += TEXT("{\n");
	OutHLSL += TEXT("    return ClothOffsets[VertexIndex];\n");
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