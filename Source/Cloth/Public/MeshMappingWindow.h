#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SComboBox.h" 
#include "IMeshMappingStrategy.h"
#include "AssetRegistry/AssetData.h" 

class USkeletalMesh;
class SEditableTextBox;

/**
 * SMeshMappingWindow
 * 负责渲染烘焙工具的 UI 界面
 */
class CLOTH_API SMeshMappingWindow : public SCompoundWidget // TODO 后续需要迁移到 Editor module 中
{
public:
    SLATE_BEGIN_ARGS(SMeshMappingWindow) {}
    SLATE_END_ARGS()

    // 构造函数
    void Construct(const FArguments& InArgs);

private:
    // --- 回调函数 (Callbacks) ---

    // 资产选择器回调
    void OnLowMeshChanged(const FAssetData& AssetData);
    void OnHighMeshChanged(const FAssetData& AssetData);

    // 资产路径回显
    FString GetLowMeshPath() const;
    FString GetHighMeshPath() const;

    // 策略选择回调
    void OnStrategySelectionChanged(TSharedPtr<IMeshMappingStrategy> NewItem, ESelectInfo::Type SelectInfo);
    TSharedRef<SWidget> GenerateStrategyComboItem(TSharedPtr<IMeshMappingStrategy> InItem);
    FText GetCurrentStrategyLabel() const;

    // 文本框回调
    void OnOutputPathChanged(const FText& NewText);
    void OnOutputNameChanged(const FText& NewText);

    // 按钮回调
    FReply OnBakeClicked();
    bool IsBakeEnabled() const; // 用于控制按钮变灰

    // --- 成员变量 (State) ---

    // UI 控件引用 (用于后续获取数据或动态刷新)
    TSharedPtr<SBox> DynamicSettingsContainer;
    TSharedPtr<SComboBox<TSharedPtr<IMeshMappingStrategy>>> StrategyComboBox;
    TSharedPtr<SEditableTextBox> OutputPathBox;
    TSharedPtr<SEditableTextBox> OutputNameBox;

    // 数据状态 (使用 WeakPtr 防止阻碍 GC)
    TWeakObjectPtr<USkeletalMesh> LowPolyMesh;
    TWeakObjectPtr<USkeletalMesh> HighPolyMesh;

    // 路径缓存
    FString SaveFolderPath;
    FString SaveFileName;

    // 策略数据源
    TArray<TSharedPtr<IMeshMappingStrategy>> AvailableStrategies;
    TSharedPtr<IMeshMappingStrategy> CurrentStrategy;
};