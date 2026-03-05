#pragma once

#include "CoreMinimal.h"
#include "OptimusComputeDataInterface.h"
#include "ComputeFramework/ComputeDataProvider.h"
#include "ClothDataInterface.generated.h"

class UClothDeformerComponent;

UCLASS(BlueprintType, EditInlineNew)
class UClothDataProvider : public UComputeDataProvider
{
	GENERATED_BODY()

public:
	// 创建渲染线程代理
	virtual FComputeDataProviderRenderProxy *GetRenderProxy() override;

	// 引用我们的组件
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth")
	TObjectPtr<UClothDeformerComponent> ClothComponent = nullptr;
};

UCLASS(BlueprintType, EditInlineNew)
class UClothDataInterface : public UOptimusComputeDataInterface
{
	GENERATED_BODY()
public:
	virtual void GetShaderParameters(TCHAR const *UID, FShaderParametersMetadataBuilder &InOutBuilder, FShaderParametersMetadataAllocations &InOutAllocations) const override;
	virtual FString GetDisplayName() const override;
	virtual TArray<FOptimusCDIPinDefinition> GetPinDefinitions() const override;
	virtual TSubclassOf<UActorComponent> GetRequiredComponentClass() const override;

	virtual void GetSupportedInputs(TArray<FShaderFunctionDefinition> &OutFunctions) const override;
	virtual void GetHLSL(FString &OutHLSL, FString const &InDataInterfaceName) const override;
	virtual UComputeDataProvider *CreateDataProvider(TObjectPtr<UObject> InBinding, uint64 InInputMask, uint64 InOutputMask) const override;
};