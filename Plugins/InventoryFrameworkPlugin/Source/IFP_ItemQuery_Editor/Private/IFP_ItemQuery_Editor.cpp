#include "IFP_ItemQuery_Editor.h"

#include "ContentBrowserContextMenu/ATA_ItemQuery.h"

#define LOCTEXT_NAMESPACE "FIFP_ItemQuery_EditorModule"

void FIFP_ItemQuery_EditorModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	InventoryFrameworkCategory = AssetTools.RegisterAdvancedAssetCategory(FName("InventoryFrameworkCategory"), FText::FromString("Inventory Framework"));

	Internal_RegisterTypeActions<FATA_ItemQuery>("Item Query");
}

void FIFP_ItemQuery_EditorModule::ShutdownModule()
{
    
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FIFP_ItemQuery_EditorModule, IFP_ItemQuery_Editor)