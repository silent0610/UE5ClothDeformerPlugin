using UnrealBuildTool;

public class ClothTests : ModuleRules
{
    public ClothTests(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "Cloth"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "UnrealEd"
            }
        );
    }
}
