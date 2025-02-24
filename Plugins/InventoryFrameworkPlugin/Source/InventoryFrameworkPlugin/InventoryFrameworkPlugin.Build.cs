// Copyright (C) Varian Daemon 2023. All Rights Reserved.

using UnrealBuildTool;

public class InventoryFrameworkPlugin : ModuleRules
{
	public InventoryFrameworkPlugin(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Latest;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core", 
				"UMG", 
				"GameFeatures", 
				"EnhancedInput",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"GameplayTags",
				"InputCore",
				"GameplayAbilities"
				// ... add private dependencies that you statically link with here ...	
			}
			);
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					//Used by the item data asset to open the item instance class
					"UnrealEd",
					"EditorScriptingUtilities",
					"ContentBrowserData"
				});
		}
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
