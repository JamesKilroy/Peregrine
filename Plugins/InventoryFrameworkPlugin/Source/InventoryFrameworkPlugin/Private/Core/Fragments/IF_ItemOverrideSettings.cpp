// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Fragments/IF_ItemOverrideSettings.h"

#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Fragments/FL_IFP_FragmentHelpers.h"
#include "Core/Items/DA_CoreItem.h"

FItemOverrideSettings UIF_ItemOverrideSettings::GetOverrideSettingsFragmentFromItem(FS_InventoryItem Item)
{
	UFL_InventoryFramework::UpdateItemStruct(Item);

	FItemOverrideSettings* OverrideFragment = FindFragment<FItemOverrideSettings>(Item.ItemFragments);
	return OverrideFragment ? *OverrideFragment : FItemOverrideSettings();
}

FItemOverrideSettings UIF_ItemOverrideSettings::GetOverrideSettingsFragmentFromItemAsset(UDA_CoreItem* ItemAsset,
	bool GetFromAssetFragments)
{
	FItemOverrideSettings* OverrideFragment = FindFragment<FItemOverrideSettings>(
	GetFromAssetFragments ? ItemAsset->ItemAssetFragments : ItemAsset->ItemStructFragments);
	
	return OverrideFragment ? *OverrideFragment : FItemOverrideSettings();
}

FItemOverrideSettings UIF_ItemOverrideSettings::ToItemOverrideSettings(FInstancedStruct Struct)
{
	if(const FItemOverrideSettings* OverrideFragment = Struct.GetPtr<FItemOverrideSettings>())
	{
		return *OverrideFragment;
	}

	return FItemOverrideSettings();
}
