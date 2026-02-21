#include "ClothEditorModule.h"
#include "MeshMappingWindow.h" // 必须引入你写的UI类
#include "Widgets/Docking/SDockTab.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "FClothEditorModule"

// 定义一个全局唯一的 Tab 标识符 (内部名字)
static const FName ClothMappingTabName("ClothMappingTab");

void FClothEditorModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("ClothEditor Module Started"));

    // 1. 向引擎全局注册一个 Nomad Tab (游离标签页，用户可以停靠在任何地方)
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
                                ClothMappingTabName,
                                FOnSpawnTab::CreateRaw(this, &FClothEditorModule::OnSpawnPluginTab))
        .SetDisplayName(LOCTEXT("ClothMappingTabTitle", "Cloth Mesh Mapping Tool"))              // 标签页上显示的文本
        .SetTooltipText(LOCTEXT("ClothMappingTooltipText", "Open the Cloth Mesh Mapping Tool.")) // 鼠标悬停提示
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetToolsCategory());                         // 将启动按钮放入 Window -> Tools 菜单下
}

void FClothEditorModule::ShutdownModule()
{
    // 2. 模块卸载时（如关闭编辑器），清理注册信息防止内存泄漏
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(ClothMappingTabName);
}

TSharedRef<SDockTab> FClothEditorModule::OnSpawnPluginTab(const FSpawnTabArgs &SpawnTabArgs)
{
    // 3. 实例化你的 Slate 界面，并把它塞进一个 SDockTab 里返回给引擎
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
            [
                // 实例化我们之前写的界面
                SNew(SMeshMappingWindow)];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FClothEditorModule, ClothEditor)