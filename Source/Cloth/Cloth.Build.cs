// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class Cloth : ModuleRules
{
	public Cloth(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        // 使用自己的ONNX Runtime 1.20版本
        string OnnxRuntimePath = Path.Combine(PluginDirectory, "ThirdParty", "OnnxRuntime");
        string includePath = Path.Combine(OnnxRuntimePath, "include");
        PublicIncludePaths.AddRange(new string[] { includePath });
        
        // 链接ONNX Runtime库文件
        string libPath = Path.Combine(OnnxRuntimePath, "lib");
        PublicAdditionalLibraries.Add(Path.Combine(libPath, "onnxruntime_cloth.lib"));
        
        // 配置运行时DLL依赖
        RuntimeDependencies.Add(Path.Combine(libPath, "onnxruntime_cloth.dll"));
        RuntimeDependencies.Add(Path.Combine(libPath, "onnxruntime_providers_shared.dll"));
        
        // 配置延迟加载DLL
        PublicDelayLoadDLLs.Add("onnxruntime_cloth.dll"));
        PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Projects",
                "MeshDescription",
                "StaticMeshDescription",
                "GeometryCore",
                "MeshConversion",
				
				// ... add other public dependencies that you statically link with here ...
			}
			);
		PublicSystemLibraries.AddRange(new string[] { "dxcore.lib", "directml.lib" });	
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
        PublicSystemLibraries.Add("dxcore.lib");
        PublicSystemLibraries.Add("d3d12.lib");
        PublicSystemLibraries.Add("DirectML.lib");
    }

}
