// Copyright Epic Games, Inc. All Rights Reserved.

#include "Cloth.h"
#include "Interfaces/IPluginManager.h"
#include "HAL/PlatformFilemanager.h"

// 包含ONNX Runtime头文件用于测试
#if PLATFORM_WINDOWS && PLATFORM_64BITS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <psapi.h>
#endif
#include "onnxruntime_cxx_api.h"
#if PLATFORM_WINDOWS && PLATFORM_64BITS
#include "Windows/HideWindowsPlatformTypes.h"
#endif

#define LOCTEXT_NAMESPACE "FClothModule"

void FClothModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("Cloth module starting - testing ONNX Runtime initialization..."));
	
	// 测试ONNX Runtime初始化
	try
	{
		// 检测版本信息
		UE_LOG(LogTemp, Warning, TEXT("=== ONNX Runtime Version Detection ==="));
		UE_LOG(LogTemp, Warning, TEXT("Header API Version: %d"), ORT_API_VERSION);
		
		// 检测实际加载的DLL模块
		HMODULE hModule = GetModuleHandleA("onnxruntime.dll");
		if (hModule)
		{
			TCHAR modulePath[MAX_PATH];
			if (GetModuleFileName(hModule, modulePath, MAX_PATH))
			{
				UE_LOG(LogTemp, Warning, TEXT("Actually loaded DLL path: %s"), modulePath);
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("onnxruntime.dll not found in loaded modules yet"));
		}
		
		// 获取API并测试基本创建
		const OrtApiBase* apiBase = OrtGetApiBase();
		if (apiBase)
		{
			const OrtApi* ortApi = apiBase->GetApi(ORT_API_VERSION);
			if (ortApi)
			{
				UE_LOG(LogTemp, Log, TEXT("ONNX Runtime API %d available"), ORT_API_VERSION);
				
				// 测试环境和会话选项创建
				Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "ClothModule");
				Ort::SessionOptions sessionOptions;
				
				UE_LOG(LogTemp, Log, TEXT("ONNX Runtime initialization successful!"));
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("Failed to get ONNX Runtime API"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to get OrtApiBase"));
		}
	}
	catch (const Ort::Exception& e)
	{
		UE_LOG(LogTemp, Error, TEXT("ONNX Runtime error: %s"), UTF8_TO_TCHAR(e.what()));
	}
	catch (const std::exception& e)
	{
		UE_LOG(LogTemp, Error, TEXT("Standard exception: %s"), UTF8_TO_TCHAR(e.what()));
	}
	catch (...)
	{
		UE_LOG(LogTemp, Error, TEXT("Unknown exception in ONNX Runtime initialization"));
	}

	// 1. 获取插件对象
	TSharedPtr<IPlugin> ClothPlugin = IPluginManager::Get().FindPlugin(TEXT("Cloth"));
	if (ClothPlugin.IsValid())
	{
		// 2. 获取插件的物理根目录 (e.g., .../Plugins/Cloth)
		FString PluginBaseDir = ClothPlugin->GetBaseDir();

		// 3. 精准拼接 Shaders 文件夹路径
		// 注意：只需拼接 "Shaders" 即可，BaseDir 已经包含了之前的路径
		FString ShaderDir = FPaths::Combine(PluginBaseDir, TEXT("Shaders"));

		// 4. 打印到 Log，方便你出错了去 Output Log 确认路径到底对不对
		UE_LOG(LogTemp, Log, TEXT("Cloth Plugin Shader Dir: %s"), *ShaderDir);

		// 5. 检查文件夹是否存在（防止崩溃）
		if (FPaths::DirectoryExists(ShaderDir))
		{
			AddShaderSourceDirectoryMapping(TEXT("/Plugin/Cloth"), ShaderDir);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Cloth Shader directory NOT found at: %s"), *ShaderDir);
		}
	}
}

void FClothModule::ShutdownModule()
{
	// 不需要释放DLL句柄 - NNERuntimeORT负责管理
}
#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FClothModule, Cloth)