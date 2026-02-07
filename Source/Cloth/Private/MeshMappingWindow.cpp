#include "MeshMappingWindow.h"
#include "Cloth/Public/MeshMappingAsset.h" // Runtime 资产定义
#include "Engine/SkeletalMesh.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "PropertyCustomizationHelpers.h" // 包含 SObjectPropertyEntryBox
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "KnnMeshMapping.h" // 包含具体的策略实现

#define LOCTEXT_NAMESPACE "SMeshMappingWindow"

void SMeshMappingWindow::Construct(const FArguments& InArgs)
{
    // 1. 初始化策略列表 (这里我们手动添加 KNN 策略)
    // 实际项目中可以遍历所有 IMeshMappingStrategy 的子类自动添加
    AvailableStrategies.Add(MakeShared<FKnnMappingStrategy>(3)); // K=3
    CurrentStrategy = AvailableStrategies[0];

    // 2. 初始化默认路径
    SaveFolderPath = TEXT("/Game/ClothData");
    SaveFileName = TEXT("NewMappingAsset");

    // 3. UI 布局
    ChildSlot
        [
            SNew(SVerticalBox)

                // --- 标题 ---
                + SVerticalBox::Slot().AutoHeight().Padding(10)
                [
                    SNew(STextBlock)
                        .Text(LOCTEXT("Title", "Snug Mesh Mapping Tool"))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                        .Justification(ETextJustify::Center)
                ]

                // --- Low Poly Mesh 选择器 ---
                + SVerticalBox::Slot().AutoHeight().Padding(5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(0, 5, 10, 0).VAlign(VAlign_Center)
                        [
                            SNew(STextBlock).Text(LOCTEXT("LowPoly", "Low Poly Mesh:"))
                        ]
                        + SHorizontalBox::Slot().FillWidth(1.0f)
                        [
                            SNew(SObjectPropertyEntryBox)
                                .AllowedClass(USkeletalMesh::StaticClass()) // 只允许选骨骼模型
                                .ObjectPath(this, &SMeshMappingWindow::GetLowMeshPath) // 回显
                                .OnObjectChanged(this, &SMeshMappingWindow::OnLowMeshChanged) // 回调
                                .AllowClear(true)
                                .DisplayUseSelected(true) // 显示“使用当前在内容浏览器中选中的资产”小箭头
                        ]
                ]

            // --- High Poly Mesh 选择器 ---
            + SVerticalBox::Slot().AutoHeight().Padding(5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(0, 5, 10, 0).VAlign(VAlign_Center)
                        [
                            SNew(STextBlock).Text(LOCTEXT("HighPoly", "High Poly Mesh:"))
                        ]
                        + SHorizontalBox::Slot().FillWidth(1.0f)
                        [
                            SNew(SObjectPropertyEntryBox)
                                .AllowedClass(USkeletalMesh::StaticClass())
                                .ObjectPath(this, &SMeshMappingWindow::GetHighMeshPath)
                                .OnObjectChanged(this, &SMeshMappingWindow::OnHighMeshChanged)
                                .AllowClear(true)
                                .DisplayUseSelected(true)
                        ]
                ]

            // --- 策略选择 ---
            + SVerticalBox::Slot().AutoHeight().Padding(5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(0, 5, 10, 0).VAlign(VAlign_Center)
                        [
                            SNew(STextBlock).Text(LOCTEXT("Strategy", "Mapping Algorithm:"))
                        ]
                        + SHorizontalBox::Slot().FillWidth(1.0f)
                        [
                            SAssignNew(StrategyComboBox, SComboBox<TSharedPtr<IMeshMappingStrategy>>)
                                .OptionsSource(&AvailableStrategies)
                                .OnGenerateWidget(this, &SMeshMappingWindow::GenerateStrategyComboItem)
                                .OnSelectionChanged(this, &SMeshMappingWindow::OnStrategySelectionChanged)
                                [
                                    SNew(STextBlock).Text(this, &SMeshMappingWindow::GetCurrentStrategyLabel)
                                ]
                        ]
                ]

            // --- 动态设置区域 (策略的参数面板) ---
            + SVerticalBox::Slot().AutoHeight().Padding(5)
                [
                    SAssignNew(DynamicSettingsContainer, SBox)
                ]

                // --- 分割线 ---
                + SVerticalBox::Slot().AutoHeight().Padding(0, 10)
                [
                    SNew(SSeparator)
                ]

                // --- 输出路径设置 ---
                + SVerticalBox::Slot().AutoHeight().Padding(5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(0, 5, 10, 0).VAlign(VAlign_Center)
                        [
                            SNew(STextBlock).Text(LOCTEXT("OutFolder", "Output Folder:"))
                        ]
                        + SHorizontalBox::Slot().FillWidth(1.0f)
                        [
                            SAssignNew(OutputPathBox, SEditableTextBox)
                                .Text(FText::FromString(SaveFolderPath))
                                .OnTextChanged(this, &SMeshMappingWindow::OnOutputPathChanged)
                        ]
                ]
            + SVerticalBox::Slot().AutoHeight().Padding(5)
                [
                    SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(0, 5, 10, 0).VAlign(VAlign_Center)
                        [
                            SNew(STextBlock).Text(LOCTEXT("OutName", "Output Name:"))
                        ]
                        + SHorizontalBox::Slot().FillWidth(1.0f)
                        [
                            SAssignNew(OutputNameBox, SEditableTextBox)
                                .Text(FText::FromString(SaveFileName))
                                .OnTextChanged(this, &SMeshMappingWindow::OnOutputNameChanged)
                        ]
                ]

            // --- Bake 按钮 ---
            + SVerticalBox::Slot().AutoHeight().Padding(10, 20)
                .HAlign(HAlign_Right)
                [
                    SNew(SButton)
                        .Text(LOCTEXT("BakeBtn", "Bake Mapping Asset"))
                        .OnClicked(this, &SMeshMappingWindow::OnBakeClicked)
                        .IsEnabled(this, &SMeshMappingWindow::IsBakeEnabled)
                        .ToolTipText(LOCTEXT("BakeTip", "Generate sparse matrix and save to asset"))
                ]
        ];

    // 初始化动态区域
    if (CurrentStrategy.IsValid())
    {
        
        DynamicSettingsContainer->SetContent(CurrentStrategy->CreateSettingsWidget()); 
    }
}

// --------------------------------------------------------
// 资产选择回调
// --------------------------------------------------------
void SMeshMappingWindow::OnLowMeshChanged(const FAssetData& AssetData)
{
    LowPolyMesh = Cast<USkeletalMesh>(AssetData.GetAsset());
}

void SMeshMappingWindow::OnHighMeshChanged(const FAssetData& AssetData)
{
    HighPolyMesh = Cast<USkeletalMesh>(AssetData.GetAsset());
}

FString SMeshMappingWindow::GetLowMeshPath() const
{
    return LowPolyMesh.IsValid() ? LowPolyMesh->GetPathName() : FString();
}

FString SMeshMappingWindow::GetHighMeshPath() const
{
    return HighPolyMesh.IsValid() ? HighPolyMesh->GetPathName() : FString();
}

// --------------------------------------------------------
// 策略 UI 回调
// --------------------------------------------------------
TSharedRef<SWidget> SMeshMappingWindow::GenerateStrategyComboItem(TSharedPtr<IMeshMappingStrategy> InItem)
{
    return SNew(STextBlock).Text(FText::FromString(InItem->GetStrategyName()));
}

void SMeshMappingWindow::OnStrategySelectionChanged(TSharedPtr<IMeshMappingStrategy> NewItem, ESelectInfo::Type SelectInfo)
{
    CurrentStrategy = NewItem;
    // 如果策略有自定义参数UI，在这里 SetContent 切换
}

FText SMeshMappingWindow::GetCurrentStrategyLabel() const
{
    return CurrentStrategy.IsValid() ? FText::FromString(CurrentStrategy->GetStrategyName()) : FText::FromString("None");
}

// --------------------------------------------------------
// 输出设置回调
// --------------------------------------------------------
void SMeshMappingWindow::OnOutputPathChanged(const FText& NewText)
{
    SaveFolderPath = NewText.ToString();
}

void SMeshMappingWindow::OnOutputNameChanged(const FText& NewText)
{
    SaveFileName = NewText.ToString();
}

// --------------------------------------------------------
// 核心逻辑：Bake & Save
// --------------------------------------------------------
bool SMeshMappingWindow::IsBakeEnabled() const
{
    return LowPolyMesh.IsValid() && HighPolyMesh.IsValid() && CurrentStrategy.IsValid();
}

FReply SMeshMappingWindow::OnBakeClicked()
{
    if (!IsBakeEnabled()) return FReply::Handled();

    // 1. 组装完整的包路径 (Package Path)
    FString PackageName = SaveFolderPath / SaveFileName;
    FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");

    // 2. 检查并清理文件名
    FString Name;
    FString PackagePath;
    AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackagePath, Name);

    // 3. 创建 Asset 包 (UPackage)
    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        // 提示错误
        return FReply::Handled();
    }
    Package->FullyLoad();

    // 4. 创建 UMeshMappingAsset 对象
    // 使用 NewObject 创建在 Package 内
    UMeshMappingAsset* NewAsset = NewObject<UMeshMappingAsset>(Package, FName(*Name), RF_Public | RF_Standalone | RF_Transactional);

    if (NewAsset)
    {
        // 5. 调用策略进行计算 (耗时操作，实际项目中建议放进异步任务或显示进度条)
        bool bSuccess = CurrentStrategy->GenerateMapping(LowPolyMesh.Get(), HighPolyMesh.Get(), NewAsset);

        if (bSuccess)
        {
            // 6. 标记脏位并通知编辑器
            NewAsset->MarkPackageDirty();
            FAssetRegistryModule::AssetCreated(NewAsset);

            // 7. 自动保存到磁盘 (可选，如果不写这句，用户需要在 Content Browser 里手动按 Save)
            // UEditorLoadingAndSavingUtils::SavePackages({Package}, true); 
        }
        else
        {
            // 计算失败，清理
            // Log Error...
        }
    }

    return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE