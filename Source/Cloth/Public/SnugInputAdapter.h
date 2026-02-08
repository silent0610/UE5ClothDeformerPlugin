#pragma once

#include "InputAdapterBase.h"
#include "Components/SkeletalMeshComponent.h"

/**
 * @class FSnugInputAdapter
 * @brief 针对 SnUG 模型的具体适配器
 * 数据需求:
 * 1. "pose"  (72 floats): 24个关节的 Axis-Angle 旋转
 * 2. "betas" (10 floats): 体型参数 (通常映射自 Morph Targets)
 * 3. "trans" (3 floats):  根骨骼位移/速度 (视模型具体训练数据而定)
 */
class CLOTH_API FSnugInputAdapter : public FInputAdapterBase
{
public:
    FSnugInputAdapter();
    virtual void Initialize(USkeletalMeshComponent* InSkelMeshComp) override;
    virtual TMap<FString, TArray<float>> ExtractInputs(float DeltaTime) override;
    virtual void Reset() override;

private:
    //brief 将 UE 的四元数转换为 PyTorch/SMPL 习惯的 Axis-Angle 向量, 同时处理坐标系转换 (Left-Handed -> Right-Handed)  不确定有没有用
    void ConvertBoneRotation(const FQuat& InQuat, float* OutDataPtr, bool bFixCoordinateSystem = true);

    // --- 缓存数据 (避免每帧查找) ---
    TWeakObjectPtr<USkeletalMeshComponent> SkelComp{};

    // 缓存后的 UE 骨骼索引 (Index Mapping)
    TArray<int32> CachedBoneIndices{};

    // 缓存上一帧位置，用于计算速度
    FVector PreviousRootLocation{ FVector::ZeroVector };
    bool bFirstFrame_{true};

    // 模型需要的骨骼名称列表 (顺序必须严格对应 Python 训练时的顺序)
    TArray<FName> TargetBoneNames{};
};