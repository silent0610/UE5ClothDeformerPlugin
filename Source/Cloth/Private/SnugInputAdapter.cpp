#include "SnugInputAdapter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

// 构造函数：硬编码 SMPL 标准骨骼层级顺序
// 警告：这个列表必须和训练模型时使用的 SMPL 定义完全一致！
FSnugInputAdapter::FSnugInputAdapter()
{
    TargetBoneNames = {
        "Pelvis",       // 0
        "L_Hip",        // 1
        "R_Hip",        // 2
        "Spine1",       // 3 (Spine)
        "L_Knee",       // 4
        "R_Knee",       // 5
        "Spine2",       // 6 (Spine1)
        "L_Ankle",      // 7
        "R_Ankle",      // 8
        "Spine3",       // 9 (Spine2)
        "L_Foot",       // 10
        "R_Foot",       // 11
        "Neck",         // 12
        "L_Collar",     // 13 (L_Clavicle)
        "R_Collar",     // 14 (R_Clavicle)
        "Head",         // 15
        "L_Shoulder",   // 16 (L_UpperArm)
        "R_Shoulder",   // 17 (R_UpperArm)
        "L_Elbow",      // 18 (L_Forearm)
        "R_Elbow",      // 19 (R_Forearm)
        "L_Wrist",      // 20 (L_Hand)
        "R_Wrist",      // 21 (R_Hand)
        "L_Hand",       // 22 (通常 SMPL 24个关节包含手掌，具体看模型是否包含手指)
        "R_Hand"        // 23
    };
    // 注意：请根据您的具体骨骼资产修改上述字符串名称！
    // 比如 Mixamo 骨骼可能叫 "Hips" 而不是 "Pelvis"
}

void FSnugInputAdapter::Initialize(USkeletalMeshComponent* InSkelMeshComp)
{
    SkelComp = InSkelMeshComp;
    CachedBoneIndices.Empty();

    if (!SkelComp.IsValid() || !SkelComp->GetSkeletalMeshAsset())
    {
        UE_LOG(LogTemp, Warning, TEXT("SnugInputAdapter: Invalid Skeletal Mesh Component"));
        return;
    }

    // 缓存骨骼索引，避免每帧用 FindBoneIndex (耗时操作)
    for (const FName& BoneName : TargetBoneNames)
    {
        int32 BoneIndex = SkelComp->GetBoneIndex(BoneName);
        if (BoneIndex == INDEX_NONE)
        {
            UE_LOG(LogTemp, Warning, TEXT("SnugInputAdapter: Missing bone '%s' on mesh!"), *BoneName.ToString());
        }
        CachedBoneIndices.Add(BoneIndex);
    }

    Reset();
}

void FSnugInputAdapter::Reset()
{
    bFirstFrame_ = true;
    PreviousRootLocation = FVector::ZeroVector;
}

TMap<FString, TArray<float>> FSnugInputAdapter::ExtractInputs(float DeltaTime)
{
    TMap<FString, TArray<float>> Inputs;

    if (!SkelComp.IsValid()) return Inputs;

    USkeletalMesh* MeshAsset = SkelComp->GetSkeletalMeshAsset();
    if (!MeshAsset) return Inputs; // 安全检查

    // 资源中获取 Reference Skeleton 
    const FReferenceSkeleton& RefSkeleton = MeshAsset->GetRefSkeleton();


    // -------------------------------------------------------------------------
    // 1. 提取 Pose (72 floats = 24 bones * 3 axis-angle)
    // -------------------------------------------------------------------------
    TArray<float>& PoseData = Inputs.Add("pose");
    PoseData.SetNumUninitialized(72); // 24 * 3

    for (int32 i = 0; i < CachedBoneIndices.Num(); i++)
    {
        int32 BoneIndex = CachedBoneIndices[i];
        float* OutputPtr = &PoseData[i * 3];

        if (BoneIndex != INDEX_NONE)
        {
            // 3. 获取当前骨骼的 Component Space Transform
            FTransform ChildTM = SkelComp->GetBoneTransform(BoneIndex, FTransform::Identity);
            // 注意：GetBoneTransform 第二个参数在某些 UE 版本是 Space，某些是 Default。
            // 更稳健的写法是显式指定空间：
            // FTransform ChildTM = SkelComp->GetSocketTransform(TargetBoneNames[i], RTS_Component);

            // 4. [关键修正] 使用 RefSkeleton 查找父骨骼索引
            int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);

            FQuat LocalQuat;

            if (ParentIndex != INDEX_NONE)
            {
                // 获取父骨骼 Transform
                FTransform ParentTM = SkelComp->GetBoneTransform(ParentIndex, FTransform::Identity);
                // 或者: SkelComp->GetSocketTransform(RefSkeleton.GetBoneName(ParentIndex), RTS_Component);

                // 计算相对旋转 (Child relative to Parent)
                // 公式: Q_local = Q_parent^(-1) * Q_child
                FTransform RelativeTM = ChildTM.GetRelativeTransform(ParentTM);
                LocalQuat = RelativeTM.GetRotation();
            }
            else
            {
                // 根骨骼处理
                LocalQuat = ChildTM.GetRotation();
            }

            // TODO 转换到模型空间 需要确认
            // ConvertBoneRotation(LocalQuat, OutputPtr, true);
        }
        else
        {
            // 骨骼丢失，填充默认值 (0,0,0) 表示无旋转
            OutputPtr[0] = 0.0f;
            OutputPtr[1] = 0.0f;
            OutputPtr[2] = 0.0f;
        }
    }

    // -------------------------------------------------------------------------
    // 2. 提取 Betas (10 floats) - 体型参数
    // -------------------------------------------------------------------------
    TArray<float>& BetasData = Inputs.Add("betas");
    BetasData.SetNumZeroed(10); // 初始化为 0 (标准身材)

    // 从 Morph Targets 或 Curves 读取 Betas
    // 假设您的骨骼上有名为 "Shape_000", "Shape_001" 等的曲线
     for (int32 i = 0; i < 10; i++)
     {
         FName CurveName = *FString::Printf(TEXT("Shape_%03d"), i);
         BetasData[i] = SkelComp->GetMorphTarget(CurveName); 
     }

    // -------------------------------------------------------------------------
    // 3. 提取 Trans (3 floats) - 根位移/速度
    // -------------------------------------------------------------------------
    // 许多 SnUG 变体需要根骨骼的 global translation 或者是 velocity
    
    TArray<float>& TransData = Inputs.Add("trans");
    TransData.SetNum(3);

    FVector CurrentRootLoc = SkelComp->GetComponentLocation(); // 或者 GetBoneLocation(Root)

    // 这里简单的填入相对位置，具体取决于模型训练时是否 normalized
    // 注意坐标系转换: UE(X,Y,Z) -> Python(X,Z,Y) or similar
    TransData[0] = CurrentRootLoc.X;
    TransData[1] = CurrentRootLoc.Z; // Swap Y/Z for typical conversion
    TransData[2] = CurrentRootLoc.Y;
    
    return Inputs;
}

void FSnugInputAdapter::ConvertBoneRotation(const FQuat& InQuat, float* OutDataOffset, bool bFixCoordinateSystem)
{
    // 1. 提取 Axis 和 Angle
    FVector Axis;
    float Angle;
    InQuat.ToAxisAndAngle(Axis, Angle);

    // 2. 处理 Axis-Angle 的周期性 (保证最小旋转路径)
    // 此时 Angle 范围是 [0, 2PI] 或 [-PI, PI]，实际上 Axis*Angle 向量长度就是弧度

    FVector ResultAxisAngle = Axis * Angle;

    if (bFixCoordinateSystem)
    {
        // [极其重要] 坐标系转换
        // 这是一个最大的坑。UE 是左手系，大多数 AI 模型是右手系。
        // 常见的转换策略是：
        // UE (X, Y, Z) -> SMPL (X, -Y, -Z) 或者 (X, Z, Y) 
        // 这完全取决于您的 .onnx 是怎么导出的。
        // 下面是一个常见的转换示例 (X-Right, Y-Up, Z-Forward vs UE):

        OutDataOffset[0] = ResultAxisAngle.X;
        OutDataOffset[1] = -ResultAxisAngle.Y; // 翻转 Y
        OutDataOffset[2] = -ResultAxisAngle.Z; // 翻转 Z
    }
    else
    {
        OutDataOffset[0] = ResultAxisAngle.X;
        OutDataOffset[1] = ResultAxisAngle.Y;
        OutDataOffset[2] = ResultAxisAngle.Z;
    }
}