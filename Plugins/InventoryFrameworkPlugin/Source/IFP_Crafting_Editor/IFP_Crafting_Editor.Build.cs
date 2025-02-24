using UnrealBuildTool;

public class IFP_Crafting_Editor : ModuleRules
{
    public IFP_Crafting_Editor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "AssetTools", 
                "IFP_Crafting",
                "InventoryFrameworkEditor",
                "InventoryFrameworkPlugin",
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "EditorStyle",
            }
        );
    }
}