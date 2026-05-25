# GPU 变形闭环计划

## 现状

```
CPU 端 (已完成):
  ExtractInputs → ONNX RunInference → ApplyMapping → UpdateMesh
                                                         │
                                                         ▼
                                               OffsetBuffer SRV (GPU VRAM)
                                               ═══════════════════════
                                               ReadClothOffset(VtxIdx) 可用
                                               ═══════════════════════
                                              
GPU 端 (缺失):
  无 ComputeShader 消费 SRV
  无 UAV 写入 SkeletalMesh 顶点缓冲区
  无 Dispatch 调用
```

已有的 Optimus 基础设施（`ClothDataInterface` / `ClothDataProvider` / `ClothComponentSource`）可提供 `ReadClothOffset(VertexIndex)` 着色器函数，但缺少实际调用的内核及 Deformer Graph 资产。

## 路线

```
Shaders/ClothDeformerKernel.usf  (新建)
      │
      ▼
Editor: 创建 UOptimusDeformer 资产，配置内核节点
      │
      ▼
ClothDeformerComponent: BeginPlay 自动加载并设置 Deformer
      │
      ▼
渲染管线: 每帧自动执行内核，SRV → 顶点位置
```

---

## 1. 创建着色器内核文件

### 文件: `Shaders/ClothDeformerKernel.usf`

```hlsl
#include "/Plugin/Cloth/ClothDeformer.ush"

// Optimus 占位符，在 DeformerGraph 中编译时会被替换为
// 数据接口实例名称 + 框架输入/输出绑定生成的函数
#ifndef CLOTH_DI_PREFIX
#define CLOTH_DI_PREFIX ClothData
#endif

[numthreads(64, 1, 1)]
void ClothDeformerKernel(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    uint VertexIndex = DispatchThreadId.x;

    // 读取 ClothDataInterface 提供的偏移
    float3 Offset = CLOTH_DI_PREFIX ## _ReadClothOffset(VertexIndex);

    // Read/Write 函数由 DeformerGraph 框架根据内核节点的
    // Position 输入/输出绑定自动生成
    float3 InPosition = ClothDeformer_Read_Position(VertexIndex);
    ClothDeformer_Write_Position(VertexIndex, InPosition + Offset);
}
```

说明：
- `[numthreads(64,1,1)]` — 每组 64 个线程，对应 64 个顶点。网格顶点总数可能很大，Optimus 会自动划分线程组。
- `ClothDeformer_Read_Position` / `ClothDeformer_Write_Position` — 这些是 Deformer Graph 框架在编译内核时根据 Position 端口的输入/输出绑定自动生成的着色器函数。内核代码本身不需要定义它们。
- `CLOTH_DI_PREFIX` — 数据接口实例名的占位符。在 DeformerGraph 中绑定数据接口后，需要将其改为实际的实例名（如 `SnugOffsets_ReadClothOffset`）。

---

## 2. 编辑器操作：创建 Deformer Graph 资产

此步骤在 UE 编辑器中手动完成一次，产出一个 `.uasset` 文件。

| 步骤 | 操作                                                                                                                                                                                                                                                                                                   |
| ---- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 2.1  | Content Browser → 右键 → Miscellaneous → Deformer Graph，命名为 `DA_ClothDeformer`                                                                                                                                                                                                                     |
| 2.2  | 双击打开 Deformer Graph，进入 Graph 编辑面板                                                                                                                                                                                                                                                           |
| 2.3  | 添加 **Kernel** 节点                                                                                                                                                                                                                                                                                   |
| 2.4  | 选中 Kernel 节点，Details 面板设置：<br>• **Kernel Source** → 选择 `ClothDeformerKernel.usf`<br>• **Entry Point** → `ClothDeformerKernel`<br>• **Thread Group Size** → 64, 1, 1<br>• **Component Source** → `Cloth Deformer Component`（即 ClothComponentSource）<br>• **Execution Domain** → `Vertex` |
| 2.5  | 在 Kernel 节点上添加 **Data Interface Input** 端口：<br>• 类型选择 `Snug Cloth Offsets`（即 ClothDataInterface）<br>• 命名为 `SnugOffsets`                                                                                                                                                             |
| 2.6  | 在 Kernel 节点上添加 **Data Input** 端口 → `Position`（从网格读取）                                                                                                                                                                                                                                    |
| 2.7  | 在 Kernel 节点上添加 **Data Output** 端口 → `Position`（写入变形后位置）                                                                                                                                                                                                                               |
| 2.8  | 保存资产 `DA_ClothDeformer`                                                                                                                                                                                                                                                                            |

注意事项：
- 由于 `{DataInterfaceName}` 是通过 `GetHLSL()` 替换的，当 Optimus 使用 `SnugOffsets` 作为实例名时，实际生成的函数为 `SnugOffsets_ReadClothOffset`。因此在内核 `.usf` 中需使用该名称，或用占位符在编译前替换。
- 如果 DeformerGraph 直接支持调用数据接口函数（无需手动拼接前缀），则内核可直接使用从 `GetSupportedInputs` 导出的函数名。此点在 2.5 实施时需验证。若 Optimus 自动将数据接口的 HLSL include 注入且保留原始函数名（无前缀），则内核直接调用 `ReadClothOffset(VertexIndex)` 即可。

---

## 3. C++ 代码修改

### 3.1 头文件变更

文件: `Source/Cloth/Public/ClothDeformerComponent.h`

新增：
```cpp
// Deformer 资产引用
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cloth Deformer|Source")
TSoftObjectPtr<class UOptimusDeformer> DeformerAsset;

// 骨骼网格缓存（避免每帧 FindComponentByClass）
TWeakObjectPtr<USkeletalMeshComponent> CachedSkelMeshComp;
```

新增 include：
```cpp
#include "Components/SkeletalMeshComponent.h"
```

### 3.2 实现变更

文件: `Source/Cloth/Private/ClothDeformerComponent.cpp`

在 `BeginPlay()` 中，Adapter 初始化之后，添加 Deformer 加载和分配：

```cpp
// 设置 Deformer（GPU 端变形管线）
CachedSkelMeshComp = GetOwner()->FindComponentByClass<USkeletalMeshComponent>();
if (!DeformerAsset.IsNull() && CachedSkelMeshComp.IsValid())
{
    UOptimusDeformer* Deformer = DeformerAsset.LoadSynchronous();
    if (Deformer)
    {
        CachedSkelMeshComp->SetMeshDeformer(Deformer);
        UE_LOG(LogTemp, Log, TEXT("ClothDeformer: Mesh Deformer assigned"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ClothDeformer: Failed to load Deformer asset"));
    }
}
else
{
    UE_LOG(LogTemp, Warning, TEXT("ClothDeformer: DeformerAsset not set or no SkeletalMeshComponent"));
}
```

在 `EndPlay()` 中清理：
```cpp
if (CachedSkelMeshComp.IsValid())
{
    CachedSkelMeshComp->SetMeshDeformer(nullptr);
}
CachedSkelMeshComp.Reset();
```

在 `TickComponent()` 中，`UpdateMesh` 调用前，将 `targetMesh` 改为缓存的 `CachedSkelMeshComp`：
```cpp
USkeletalMeshComponent* targetMesh = CachedSkelMeshComp.Get();
```

### 3.3 模块依赖确认

文件: `Source/Cloth/Cloth.Build.cs`

确认已有依赖（已经包含）：
- `"ComputeFramework"` — UOptimusDeformer 所在模块
- `"OptimusCore"` — UOptimusDeformer 定义模块

---

## 4. 数据流验证

完整链路验证：

```
每帧 Tick:
  ① ExtractInputs → ② RunInference → ③ ApplyMapping → ④ UpdateMesh (上传 SRV)
  ⑤ Optimus DeformerGraph 自动执行:
      ┌─ 读取 SkeletalMesh::Position(SRV)
      ├─ 读取 OffsetBuffer::ClothOffsets(SRV) ← ClothDataInterface
      ├─ Position + Offset → DeformedPosition
      └─ 写入 SkeletalMesh::Position(UAV) ← Deformer 框架提供
```

验证清单：
- [ ] 在编辑器中将 `DA_ClothDeformer` 指定到组件的 `DeformerAsset` 属性
- [ ] PIE 运行，观察角色服装是否随动画产生变形
- [ ] 验证 Reset 后状态是否清零
- [ ] 验证移除 DeformerAsset 后不崩溃（BeginPlay 会打印 Warning）
- [ ] 性能：在 `stat gpu` 中确认 ComputeShader 耗时在可接受范围

---

## 5. 可选优化（后续）

- [ ] 将 `DeformerAsset` 改为硬编码默认路径（`/Game/ClothData/DA_ClothDeformer`），减少用户配置步骤
- [ ] 支持 LOD 切换时偏移数组重新映射（当前 `OffsetBuffer` 大小固定为高模顶点数，切换 LOD 可能导致越界）
- [ ] 将 `FVector → FVector3f` 转换移到 `ApplyMapping` 中，消除每帧拷贝
