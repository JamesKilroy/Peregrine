#include "IFP_Crafting_Editor.h"

#include "ContentBrowserContextMenu/ATA_CraftEvent.h"
#include "ContentBrowserContextMenu/ATA_CraftingAsset.h"
#include "ContentBrowserContextMenu/ATA_RecipeData.h"
#include "ContentBrowserContextMenu/ATA_RecipeDisplay.h"
#include "ContentBrowserContextMenu/ATA_RecipeRequirement.h"

#define LOCTEXT_NAMESPACE "FIFP_Crafting_EditorModule"

void FIFP_Crafting_EditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	InventoryFrameworkCategory = AssetTools.RegisterAdvancedAssetCategory(FName("InventoryFrameworkCategory"), FText::FromString("Inventory Framework"));

	Internal_RegisterTypeActions<FATA_CraftingAsset>("Crafting Recipe Asset");
	Internal_RegisterTypeActions<FATA_RecipeDisplay>("Recipe Display");
	Internal_RegisterTypeActions<FATA_RecipeRequirement>("Recipe Requirement");
	Internal_RegisterTypeActions<FATA_CraftEvent>("Recipe Craft Event");
	Internal_RegisterTypeActions<FATA_RecipeData>("Recipe Data");
}

void FIFP_Crafting_EditorModule::ShutdownModule()
{
	
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FIFP_Crafting_EditorModule, IFP_Crafting_Editor)