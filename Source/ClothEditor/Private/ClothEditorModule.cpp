#include "ClothEditorModule.h"

#define LOCTEXT_NAMESPACE "FClothEditorModule"

void FClothEditorModule::StartupModule()
{
    // 模块启动逻辑
    // 之后我们会在这个函数里注册 Toolbar 按钮和 Detail Customization
    UE_LOG(LogTemp, Log, TEXT("ClothEditor Module Started"));
}

void FClothEditorModule::ShutdownModule()
{
    // 模块关闭逻辑，清理注册的按钮等
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FClothEditorModule, ClothEditor)