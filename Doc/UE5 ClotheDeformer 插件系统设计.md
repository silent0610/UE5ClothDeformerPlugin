---
aliases:
domain:
tags:
modifiedDate: 2026/03/01, 13:25:01
---

# UE5 ClotheDeformer 插件系统设计

## 概要设计

### 架构模式

UE5 默认是模块化和分层的

### 技术选型

- UE5.6
- C++ 17
- 三方库 ONNX
- 映射算法 KD-Tree + KNN

### 模块划分

- `Editor Module` (负责烘焙映射) `UnrealSnugEditor`
- Runtime Module 运行时推理 `clothDeformerComponent`

### 架构图

>mermaid 代码, 浏览器中搜 mermaid

```mermaid
graph TD

    %% ==========================================

    %% 样式定义

    %% ==========================================

    classDef editorModule fill:#f3e5f5,stroke:#7b1fa2,stroke-width:2px;

    classDef runtimeModule fill:#e3f2fd,stroke:#1565c0,stroke-width:2px;

    classDef dataLayer fill:#c8e6c9,stroke:#2e7d32,stroke-width:2px,stroke-dasharray: 5 5;

    classDef thirdParty fill:#fff9c4,stroke:#fbc02d,stroke-width:2px;

    classDef engineLayer fill:#eeeeee,stroke:#666666,stroke-width:1px;

  

    %% ==========================================

    %% 1. 第三方依赖 (Third Party)

    %% ==========================================

    subgraph ThirdParty [Module: ThirdParty]

        OnnxLib[("ONNX Runtime\n(DLL & Headers)")]:::thirdParty

    end

  

    %% ==========================================

    %% 2. 编辑器模块 (生产环境 - Editor Only)

    %% ==========================================

    subgraph Editor_Module [Module: UnrealSnugEditor]

        direction TB

        %% UI 层

        MapUI[Mapping Window UI]:::editorModule

        %% 逻辑层 (策略模式)

        subgraph Baking_Core [Baking Subsystem]

            StrategyBase{{"IMeshMappingStrategy\n(Interface)"}}:::editorModule

            AlgoKNN[KNN Strategy]:::editorModule

            AlgoBary[Barycentric Strategy]:::editorModule

        end

        %% 交互

        MapUI -->|Selects & Configs| StrategyBase

        %% [修复点] 使用合法的虚线箭头表示实现关系

        AlgoKNN -.->|Implements| StrategyBase

        AlgoBary -.->|Implements| StrategyBase

    end

  

    %% ==========================================

    %% 3. 数据层 (持久化资产 - The Bridge)

    %% ==========================================

    subgraph Data_Layer [Data Assets .uasset]

        direction TB

        ModelAsset[("Model Asset\n(Binary + Meta)")]:::dataLayer

        MapAsset[("Mapping Asset\n(Indices + Weights)")]:::dataLayer

    end

  

    %% ==========================================

    %% 4. 运行时模块 (游戏环境 - Runtime)

    %% ==========================================

    subgraph Runtime_Module [Module: UnrealSnug]

        direction TB

        %% 组件 (指挥官)

        Comp[ClothDeformerComponent]:::runtimeModule

        %% 适配器 (输入处理)

        Adapter[("Input Adapter\n(UE -> Tensor Map)")]:::runtimeModule

        %% 推理核心 (状态机)

        Infer[("Model Instance\n(ONNX Session + State)")]:::runtimeModule

        %% 变形核心 (计算)

        Deformer[("Sparse Matrix Core\n(CPU Calculation)")]:::runtimeModule

        %% 运行时逻辑流

        Comp -->|1. Get Data| Adapter

        Comp -->|2. Run AI| Infer

        Comp -->|3. Apply| Deformer

    end

  

    %% ==========================================

    %% 5. UE5 宿主环境

    %% ==========================================

    subgraph UE5_Host [Unreal Engine 5 Core]

        AnimSys[Animation System]:::engineLayer

        RenderSys[RHI / Render Pipeline]:::engineLayer

    end

  

    %% ==========================================

    %% 跨层级数据流 (Data Flow)

    %% ==========================================

    %% 生产流程

    Baking_Core -->|Generates| MapAsset

    ModelAsset -.->|Imported From| ThirdParty

  

    %% 消费流程

    Infer -->|Loads| ModelAsset

    Infer -->|Calls| OnnxLib

    Deformer -->|Loads| MapAsset

    %% 引擎交互

    AnimSys -->|Bone Pose| Comp

    Deformer -->|Vertex Buffer| RenderSys
```

### 实体关系图

```mermaid
erDiagram
    %% --- 1. 核心控制器 ---
    UClothDeformerComponent {
        bool bIsActive "激活状态"
        float CurrentTime "当前时间"
        TArray HiddenState "RNN记忆"
    }

    %% --- 2. 运行时实例 ---
    FOnnxModelInstance {
        VoidPtr OrtSession "ONNX会话句柄"
        
    }
    
    FSparseMappingMatrix {
    }

    %% --- 3. 静态数据资产 ---
    UClothDeformationModelAsset {
        TArray_uint8 BinaryData "字节流"
        TArray_String InputNodes "输入节点名"
    }

    UMeshMappingAsset {
        FMappingData Data "映射数据"
        FString SourceHash "Hash校验"
    }

    %% --- 4. 关系 ---
    UClothDeformerComponent ||--|| FOnnxModelInstance : Manages
    UClothDeformerComponent ||--|| FSparseMappingMatrix : Manages
    UClothDeformerComponent }|--|| UClothDeformationModelAsset : Configures
    UClothDeformerComponent }|--|| UMeshMappingAsset : Configures
    FOnnxModelInstance }|..|| UClothDeformationModelAsset : Reads
    FSparseMappingMatrix }|..|| UMeshMappingAsset : Reads
```

## 详细设计

### 时序图

```mermaid
sequenceDiagram

    autonumber

  

    participant Engine as UE5 Engine (GameThread)

    participant Comp as ClothDeformerComponent

    participant Adapter as SnugInputAdapter

    participant Instance as OnnxModelInstance

    participant Mapper as FSparseMappingMatrix (Asset)

    participant RHI as RenderCommandQueue

  

    Note over Engine, RHI: 每一帧 (Tick) 的执行流程

  

    %% 1. 触发更新

    Engine->>Comp: TickComponent(DeltaTime)

    activate Comp

  

    %% 2. 准备物理输入 (Adapter 负责翻译)

    Comp->>Adapter: ExtractInputs(DeltaTime)

    activate Adapter

    Note right of Adapter: 1. 获取 Pose (Axis-Angle)<br/>2. 获取 Betas/Trans<br/>3. 坐标系转换 (UE->SMPL)

    Adapter-->>Comp: Return TMap Inputs ("pose", "betas"...)

    deactivate Adapter

  

    %% 3. [核心变化] 注入隐藏层状态 (Component 负责记忆)

    opt First Frame

        Comp->>Comp: Init CurrentHiddenState (Zero)

    end

    Note right of Comp: 将 CurrentHiddenState<br/>作为 "hidden_in" 加入 Inputs Map

  

    %% 4. 执行推理 (Instance 负责执行)

    Comp->>Instance: Run(Inputs, Outputs)

    activate Instance

    Note right of Instance: 1. TMap -> Ort::Tensor<br/>2. Session->Run()<br/>3. Ort::Tensor -> TMap

    Instance-->>Comp: Return true (Outputs 填充完毕)

    deactivate Instance

  

    %% 5. [核心变化] 状态闭环 (Ping-Pong)

    Note right of Comp: 从 Outputs["hidden_out"]<br/>更新 CurrentHiddenState

  

    %% 6. 执行映射 (Low Poly -> High Poly)

    Note right of Comp: 从 Outputs["offsets"] 提取低模位移

    Comp->>Mapper: ApplyMapping(LowPolyOffsets, HighPolyOffsets)

    activate Mapper

    Note right of Mapper: 稀疏矩阵乘法 (CPU)<br/>High = Matrix * Low

    Mapper-->>Comp: Fill HighPolyOffsets

    deactivate Mapper

  

    %% 7. 更新渲染

    Comp->>Comp: UpdateMeshVertices(HighPolyOffsets)

    Note right of Comp: 写入 USkinnedMeshComponent<br/>的 RenderData (Position Buffer)

    Comp->>RHI: MarkRenderStateDirty() / Commit

    deactivate Comp

  

    Note over RHI: GPU 渲染管线读取新顶点
```

### 类图

```mermaid
classDiagram
    %% =========================================================
    %% 1. Runtime Module (Runtime 模块 - 核心逻辑)
    %% =========================================================
    namespace Runtime_Module {

        %% [Component] 核心组件
        class UClothDeformerComponent {
            -TUniquePtr~FOnnxModelInstance~ modelInstance_
            -TUniquePtr~FInputAdapterBase~ InputAdapter
            +UClothDeformationModelAsset* modelAsset_
            +UMeshMappingAsset* MappingAsset
            -TArray~float~ CurrentHiddenState
            -int32 HiddenLayerSize
            +bool Initialize()
            +void TickComponent(float DeltaTime, ...)
            +bool RunInference(...)
            +void Reset()
        }

        %% [Adapter Base] 适配器基类
        class FInputDataAdapterBase {
            <<Abstract>>
            +Initialize(USkeletalMeshComponent*)*
            +TMap~FString, TArray_float~ ExtractInputs(float DeltaTime)*
            +Reset()*
        }

        %% [Snug Adapter] 具体适配器
        class FSnugInputAdapter {
            -TWeakObjectPtr~USkeletalMeshComponent~ SkelComp
            -TArray~int32~ CachedBoneIndices
            -TArray~FName~ TargetBoneNames
            +ExtractInputs(float DeltaTime) override
            -void ConvertBoneRotation(...)
        }

        %% [Instance] 模型推理实例
        class FOnnxModelInstance {
            -TUniquePtr~Ort::Session~ session_
            -TUniquePtr~Ort::Env~ env_
            +FOnnxModelInstance(UClothDeformationModelAsset*)
            +bool Run(const TArray~float~& Input, TArray~float~& Output)
            +bool Run(const TMap~Inputs~, TMap~Outputs~)
        }

        %% [Matrix Struct] 稀疏矩阵核心数据结构
        class FSparseMappingMatrix {
            <<Struct>>
            +TArray~int32~ RowPtr
            +TArray~int32~ ColIndice
            +TArray~float~ Value
            +void SetFromTriplet(...)
            +bool ApplyMapping(TArray~FVector~ In, TArray~FVector~ Out)
        }

        %% [Model Asset] ONNX 模型数据资产
        class UClothDeformationModelAsset {
            +TArray~uint8~ modelData_
            +TArray~FString~ inputNodeNames_
            +TArray~FString~ outputNodeNames_
            +void LoadModelData()
        }

        %% [Mapping Asset] 映射数据资产
        class UMeshMappingAsset {
            +FSparseMappingMatrix MappingData
        }
    }

    %% =========================================================
    %% 2. Editor Module (Editor 模块 - 工具与生产)
    %% =========================================================
    namespace Editor_Module {

        %% [UI] 烘焙工具窗口
        class SMeshMappingWindow {
            -TWeakObjectPtr~USkeletalMesh~ LowPolyMesh
            -TWeakObjectPtr~USkeletalMesh~ HighPolyMesh
            -TSharedPtr~IMeshMappingStrategy~ CurrentStrategy
            -TArray~TSharedPtr_IMeshMappingStrategy~ AvailableStrategies
            -TSharedPtr~SBox~ DynamicSettingsContainer
            +void Construct(const FArguments&)
            -FReply OnBakeClicked()
        }

        %% [Strategy Interface] 映射策略接口
        class IMeshMappingStrategy {
            <<Interface>>
            +FSparseMappingMatrix BuildMappingMatrix(...)*
            +bool GenerateMapping(..., UMeshMappingAsset* OutAsset)*
            +TSharedRef~SWidget~ CreateSettingsWidget()*
            +FString GetStrategyName()*
        }

        %% [KNN Strategy] 具体策略实现
        class FKnnMappingStrategy {
            -int32 k_
            +GenerateMapping(...) override
            +CreateSettingsWidget() override
        }
    }

    %% =========================================================
    %% 关系定义
    %% =========================================================

    %% --- Runtime 组合关系 ---
    UClothDeformerComponent *-- FInputDataAdapterBase : Owns (UniquePtr)
    UClothDeformerComponent *-- FOnnxModelInstance : Owns (UniquePtr)
    UClothDeformerComponent --> UClothDeformationModelAsset : References
    UClothDeformerComponent --> UMeshMappingAsset : References

    %% --- Adapter 继承 ---
    FInputDataAdapterBase <|-- FSnugInputAdapter : Inherits

    %% --- Data 依赖 ---
    FOnnxModelInstance ..> UClothDeformationModelAsset : Reads Data
    UMeshMappingAsset *-- FSparseMappingMatrix : Contains

    %% --- Editor 聚合与依赖 ---
    SMeshMappingWindow o-- IMeshMappingStrategy : Holds List
    FKnnMappingStrategy --|> IMeshMappingStrategy : Inherits
    
    %% --- Editor 生产 Runtime 资产 ---
    IMeshMappingStrategy ..> UMeshMappingAsset : Generates/Writes
    IMeshMappingStrategy ..> FSparseMappingMatrix : Builds
```
