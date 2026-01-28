// ClothDeformerComponent.h

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ClothDeformationModelAsset.h"
#include "OnnxModelInstance.h"
#include "SparseMappingMatrix.h"
#include "ClothDeformerComponent.generated.h"

// Forward-declare our classes
class UClothDeformationModelAsset;
class FOnnxModelInstance;


UENUM(BlueprintType)
enum class EMappingStrategyType : uint8
{
	KNN UMETA(DisplayName = "KNN (K-Nearest Neighbors)"),
	SurfaceProjection UMETA(DisplayName = "Surface Projection (AABB)")
};

/**
 * UClothDeformerComponent
 * 一个ActorComponent，为ONNX模型推理提供了易于使用的接口。
 * 它持有一个对UClothDeformationModelAsset的引用，并管理FOnnxModelInstance的生命周期以在游戏过程中执行推理。
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class CLOTH_API UClothDeformerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClothDeformerComponent();
	virtual ~UClothDeformerComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	// Called when the game ends
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// 使用当前指定的ModelAsset初始化推理引擎。
	// 如果成功则返回true。可以调用此函数在运行时切换模型。
	UFUNCTION(BlueprintCallable, Category = "Cloth Deformer")
	bool Initialize();

	// 使用加载的模型运行推理。
	UFUNCTION(BlueprintCallable, Category = "Cloth Deformer")
	bool RunInference(const TArray<float> &InputData, TArray<float> &OutputData);

	// 检查模型是否已加载并准备好进行推理。
	UFUNCTION(BlueprintCallable, Category = "Cloth Deformer")
	bool IsInitialized() const;

	// 该组件应用的 ONNX 模型资产
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth Deformer|Source")
	UClothDeformationModelAsset *modelAsset_{nullptr};

	// 预计算的映射资产
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth Deformer|Source")
	class UMeshMappingAsset* MappingAsset{nullptr};

	// --- Debug / Testing ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth Deformer|Debug")
	TArray<float> TestInputArray;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Cloth Deformer|Debug")
	TArray<float> TestOutputArray;
	
	// --- Editor Only: Mapping Generation ---

	// 用于测试MeshMapping的低分辨率网格模型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Mapping|Source")
	UStaticMesh *testLowMesh_{nullptr};

	// 用于测试MeshMapping的高分辨率网格模型
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Mapping|Source")
	UStaticMesh *testHighMesh_{nullptr};
	
	// 选择用于构建映射的策略
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Mapping|Source")
	EMappingStrategyType MappingStrategy = EMappingStrategyType::KNN;

	// 临时存储计算出的映射矩阵，不会被保存。
	UPROPERTY(VisibleInstanceOnly, Transient, Category = "Mesh Mapping|Result")
	FSparseMappingMatrix BuiltMappingMatrix;

	UFUNCTION(CallInEditor, Category = "Mesh Mapping|Actions")
	void BuildMappingMatrix();

	UFUNCTION(CallInEditor, Category = "Mesh Mapping|Actions")
	void SaveMappingToAsset();

    UFUNCTION(CallInEditor, Category = "Mesh Mapping|Actions")
    void GenerateAndSaveMappingAsset();

	// 重置模型和清理资源
	UFUNCTION(BlueprintCallable, Category = "Cloth Deformer")
	void Reset();

private:
	bool bIsInitialized{false};
	// 执行实际推理的模型的运行时实例。
	TUniquePtr<FOnnxModelInstance> modelInstance_{nullptr};
};