#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FClothEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

};
/*
这是一个非常明智的决定。在进行物理文件移动之前，先明确 **ClothEditor（编辑器模块）** 的职责蓝图，可以让你对架构有更清晰的掌控。

以下是为您准备的 **ClothEditor 模块架构设计说明书**。这段文本可以用作您的开发文档或重构指南。

---

### **ClothEditor 模块架构设计与职责说明**

#### **1. 核心定位 (Core Mission)**

`ClothEditor` 模块是本插件的 **“生产车间”**。它只在 Unreal Editor 开发环境下加载，负责提供所有 **可视化工具**、**资产生成** 和 **数据烘焙** 功能。它的存在是为了保证 Runtime 模块（`Cloth`）的纯净与高效，确保打包后的游戏不包含任何臃肿的编辑器代码。

#### **2. 未来承担的具体职责 (Responsibilities)**

* **UI 交互层 (The Frontend)**
* **工具窗口 (`SMeshMappingWindow`)**：负责绘制 Slate 界面，包括模型选择器、参数输入框、进度条和烘焙按钮。
* **属性面板定制 (Details Customization)**：未来可能负责自定义 `UClothDeformerComponent` 在编辑器中的显示方式（例如，直接在组件面板上添加一个 "Open Mapping Tool" 的快捷按钮）。
* **菜单集成**：负责将工具注册到 UE 编辑器的 "Tools" 菜单或工具栏中。


* **核心逻辑层 (The Backend)**
* **网格处理算法**：包含所有依赖 `GeometryCore` 和 `MeshDescription` 的重型几何算法（如 KNN 搜索、射线投射、重心坐标计算）。
* **资产烘焙 (Baking)**：负责调用算法生成 `FSparseMappingMatrix` 数据。
* **资产读写 (IO)**：负责创建新的 `.uasset` 文件（`UMeshMappingAsset`），并将计算结果序列化写入磁盘。这涉及对 `AssetTools` 和 `UnrealEd` 的调用。



#### **3. 未来需要执行的具体修改 (Action Plan)**

当您准备好执行第 5 和第 6 步时，需要按照以下清单进行修改：

**A. 文件迁移 (Migration)**
需要将以下文件从 `Source/Cloth` 移动到 `Source/ClothEditor`：

* `SMeshMappingWindow.h/.cpp` (界面)
* `IMeshMappingStrategy.h` (策略接口)
* `KnnMeshMapping.h/.cpp` (具体策略实现)
* *(未来新增的所有烘焙策略都应放在这里)*

**B. 代码清理 (Cleanup in Runtime)**
`UClothDeformerComponent` (Runtime) 需要进行“瘦身”：

* **删除** 所有 `CallInEditor` 函数（如 `BuildMappingMatrix`, `SaveMappingToAsset`）。
* **删除** 对 `UStaticMesh` 的直接引用（`testLowMesh_`, `testHighMesh_`），因为运行时不需要原始网格，只需要烘焙好的 Asset。
* **移除依赖**：在 `Cloth.Build.cs` 中移除 `UnrealEd`, `Slate`, `GeometryCore` 等编辑器专用模块。

**C. API 宏更新 (Re-branding)**
移动后的头文件类声明宏必须更改，以确保链接器能正确导出符号：

* 原：`class CLOTH_API SMeshMappingWindow`
* 改：`class CLOTHEDITOR_API SMeshMappingWindow`

**D. 模块依赖建立 (Linking)**

* `ClothEditor.Build.cs` 必须添加 `PublicDependencyModuleNames.Add("Cloth")`。
* *原因*：Editor 需要引用 Runtime 中定义的 `UMeshMappingAsset` 和 `FSparseMappingMatrix` 数据结构，以便向其中填充数据。



---

### **架构关系图解**

为了帮助理解，未来的数据流向将变为：

1. **开发者** 在 `ClothEditor` 的窗口中点击 "Bake"。
2. `ClothEditor` 读取原始网格，计算出矩阵。
3. `ClothEditor` 创建并写入一个 `UMeshMappingAsset` (这个类定义在 Runtime 模块)。
4. **保存** 资产到磁盘。
5. **运行时**，`Cloth` 模块的 `Component` 仅仅是 **加载 (Load)** 这个资产并使用它，完全不知道它是怎么算出来的。

这份说明明确了 Editor 模块作为“工具提供者”的角色，而 Runtime 模块则回归纯粹的“执行者”角色。

*/