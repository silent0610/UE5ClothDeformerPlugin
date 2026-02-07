这是一份关于 **Snug Cloth Plugin** 最终架构的完整技术文档。

该插件采用 **双模块架构 (Dual-Module Architecture)**，严格区分了 **运行时 (Runtime)** 和 **编辑器 (Editor)** 的职责。这种设计确保了打包后的游戏包体最小化，同时为开发者提供了强大的编辑器工具。

---

### 1. 运行时模块 (Module: Cloth)

**定位**：核心产品。负责在游戏运行期间执行推理和网格变形。
**依赖**：`Core`, `Engine`, `OnnxRuntime`。
**打包状态**：包含在最终游戏包 (Shipping Build) 中。

#### A. 核心组件 (The Conductor)

* **`UClothDeformerComponent`**
* **作用**：总指挥。挂载在角色 Actor 上，协调各子系统的运作。
* **核心职责**：
1. **生命周期管理**：负责初始化 Adapter 和 ModelInstance。
2. **状态管理 (State Owner)**：持有并维护神经网络的 **Hidden State (隐藏层状态)**（针对 RNN/GRU 模型）。
3. **Tick 循环**：每一帧调用 `Adapter->ExtractInputs` -> `Instance->Run` -> `Mapping->Apply` 的流水线。





#### B. 数据资产 (The Blueprints)

* **`UClothDeformationModelAsset`**
* **作用**：神经网络的容器。
* **数据**：存储 `.onnx` 文件的二进制数据 (`modelData_`) 以及输入输出节点的名称元数据。
* **意义**：它是 `FOnnxModelInstance` 初始化的数据源。


* **`UMeshMappingAsset`**
* **作用**：几何映射数据的容器。
* **数据**：封装了 `FSparseMappingMatrix` 结构体。
* **意义**：存储了从“低模推理结果”到“高模渲染顶点”的权重矩阵。



#### C. 逻辑单元 (The Workers)

* **`FOnnxModelInstance`**
* **作用**：通用的推理引擎。
* **职责**：封装 `Ort::Session`。它是一个**无状态的执行器**，接收 `TMap` 输入，执行 ONNX 推理，返回 `TMap` 输出。它不关心输入是骨骼还是图片，只关心 Tensor 数据。


* **`FSnugInputAdapter`** (继承自 `FInputAdapterBase`)
* **作用**：翻译官。
* **职责**：将 UE5 的游戏世界数据（`USkeletalMeshComponent` 的骨骼旋转、Morph Target 权重、根坐标）翻译成神经网络需要的数学格式（Axis-Angle, Normalized Floats）。
* **关键逻辑**：处理 UE (左手系) 到 Python/SMPL (右手系) 的坐标系转换。


* **`FSparseMappingMatrix`**
* **作用**：数学工具。
* **职责**：定义了稀疏矩阵的存储格式 (CSR/COO)。提供 `ApplyMapping` 函数，执行  的矩阵乘法运算。



---

### 2. 编辑器模块 (Module: ClothEditor)

**定位**：生产工具。负责生成数据、烘焙资产和提供可视化界面。
**依赖**：`UnrealEd`, `Slate`, `AssetTools`, `GeometryCore` 以及 **`Cloth` 模块**。
**打包状态**：**不** 包含在最终游戏包中 (Editor Only)。

#### A. 用户界面 (The Frontend)

* **`SMeshMappingWindow`**
* **作用**：烘焙工具的主界面。
* **职责**：
1. 渲染资产选择器 (`SObjectPropertyEntryBox`) 供用户选择高模和低模。
2. 渲染策略选择下拉框。
3. 提供 "Bake" 按钮，处理文件路径逻辑，调用策略生成资产。





#### B. 策略系统 (The Logic Abstraction)

* **`IMeshMappingStrategy`** (接口)
* **作用**：定义了“如何计算映射”的标准。
* **职责**：
1. `GenerateMapping`: 执行计算并写入 Asset。
2. `CreateSettingsWidget`: 允许不同的策略拥有自己独特的参数设置面板 (UI)。




* **`FKnnMappingStrategy`** (具体实现)
* **作用**：基于 K-近邻算法的映射实现。
* **职责**：
1. 使用 `GeometryCore` 的八叉树 (Octree) 快速查找高模顶点周围最近的 K 个低模顶点。
2. 计算重心坐标或距离权重。
3. 生成 `FSparseMappingMatrix` 数据。





---

### 3. 数据流全景图 (Data Pipeline)

#### **阶段一：开发期 (Editor)**

1. **用户** 打开 `SMeshMappingWindow`。
2. **选择** 低模 (Driver Mesh) 和 高模 (Render Mesh)。
3. **点击 Bake** -> 调用 `FKnnMappingStrategy`。
4. **生成** `UMeshMappingAsset` (存储了稀疏矩阵) 并保存到磁盘。

#### **阶段二：运行期 (Runtime Tick)**

1. **Input**: `UClothDeformerComponent` 呼叫 `Adapter`。
* *数据*: 骨骼旋转 (Quat) -> 转换 -> Axis-Angle Float Array。


2. **Inference**: Component 将 **Input** + **上一帧 HiddenState** 喂给 `ModelInstance`。
* *执行*: ONNX Runtime 算出低模的顶点位移 (Offsets) + 新的 HiddenState。


3. **Update State**: Component 更新自己的 HiddenState 缓存。
4. **Mapping**: Component 呼叫 `FSparseMappingMatrix::ApplyMapping`。
* *计算*: `低模位移 (2000点)` x `矩阵` = `高模位移 (50000点)`。


5. **Render**: Component 将高模位移写入渲染管线，布料产生形变。

这个架构保证了 **高内聚、低耦合**，不仅现在能跑通 SnUG 模型，未来扩展其他 AI 布料模型或更换映射算法也无需重构核心代码。