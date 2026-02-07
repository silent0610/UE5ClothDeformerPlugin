using UnrealBuildTool;

public class ClothEditor : ModuleRules
{
    public ClothEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                // 基础模块
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
                
                // 编辑器专用模块 (这些在 Runtime 里是禁用的)
                "UnrealEd",
                "ToolMenus",       // 用于添加菜单/工具栏按钮
                "AssetTools",      // 用于创建资产
                "EditorStyle",     // 用于编辑器图标和样式
                "PropertyEditor",  // 用于 SObjectPropertyEntryBox
                "GeometryCore",    // 用于几何计算 (DynamicMesh)
                
                // 依赖你的 Runtime 模块
                "Cloth" 
            }
        );
    }
}