using UnrealBuildTool;

public class IFP_ItemQuery : ModuleRules
{
    public IFP_ItemQuery(ReadOnlyTargetRules Target) : base(Target)
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
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore", 
                "InventoryFrameworkPlugin",
                "GameplayTags",
            }
        );
    }
}