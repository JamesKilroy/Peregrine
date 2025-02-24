// Copyright (C) Varian Daemon 2023. All Rights Reserved.

using UnrealBuildTool;

public class InventoryFrameworkEditor : ModuleRules
{
	public InventoryFrameworkEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"InventoryFrameworkPlugin", 
			"AssetTools", "StructUtilsEditor"
		});

		PrivateDependencyModuleNames.AddRange(new string[] 
		{
			"Core", 
			"CoreUObject", 
			"Engine", 
			"DetailCustomizations", 
			"Slate", 
			"SlateCore", 
			"PropertyEditor", 
			"EditorStyle",
			"GameplayTags", 
			"Blutility",
			"UnrealEd",
			"DeveloperSettings",
			"UMG",
			"UMGEditor",
			"EditorFramework",
			"Slate",
			"SlateCore",
			"Projects",
		});
	}
}
