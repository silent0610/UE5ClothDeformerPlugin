// ClothDeformationModelAsset.h

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ClothDeformationModelAsset.generated.h"

/**
 * UClothDeformationModelAsset
 * 这个类代表了内容浏览器中的ONNX模型资产。
 * 它的主要职责是存放指向.onnx文件的路径以及任何预解析的元数据。
 * 它是创建运行时推理实例的数据源。
 */
UCLASS(BlueprintType, meta = (DisplayName = "Cloth Deformation Model Asset"))
class CLOTH_API UClothDeformationModelAsset : public UDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
	// 指向.onnx模型文件的路径。使用FFilePath可以在编辑器中获得一个文件选择器。
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cloth Deformation Model", meta = (FilePathFilter = "ONNX Files (*.onnx)|*.onnx"))
	FFilePath modelFile_;
#endif

	// 保存ONNX模型文件内容的字节数组。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cloth Deformation Model")
	TArray<uint8> modelData_;

	// 模型的输入节点名称。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cloth Deformation Model|Metadata")
	TArray<FString> inputNodeNames_;

	// 模型的输出节点名称。
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Cloth Deformation Model|Metadata")
	TArray<FString> outputNodeNames_;

#if WITH_EDITOR
	// 当属性在编辑器中被修改后，这个函数会被调用。
	virtual void PostEditChangeProperty(FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

private:
#if WITH_EDITOR
	struct FOnnxMetadata
	{
		TArray<FString> InputNames;
		TArray<FString> OutputNames;
		bool bIsValid = false;
	};

	// 从文件路径加载二进制数据
	static TArray<uint8> LoadModelBytes(const FString &FilePath);

	// 解析内存中的元数据
	static FOnnxMetadata ParseModelMetadata(const TArray<uint8> &InModelData);
#endif
};
