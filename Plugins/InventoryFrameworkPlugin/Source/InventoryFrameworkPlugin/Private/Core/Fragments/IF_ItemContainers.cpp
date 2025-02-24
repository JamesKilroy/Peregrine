// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Fragments/IF_ItemContainers.h"

#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Fragments/FL_IFP_FragmentHelpers.h"
#include "Core/Items/DA_CoreItem.h"

bool FItemContainersFragment::IsCompatibleWithItem(UObject* ItemOwner, const FS_InventoryItem& Item)
{
	if(Item.ItemAssetSoftReference.IsNull())
	{
		return false;
	}

	if(!Item.ItemAssetSoftReference.LoadSynchronous()->GetDefaultContainers().IsValidIndex(0))
	{
		return false;
	}
	
	return FItemFragment::IsCompatibleWithItem(ItemOwner, Item);
}

FItemContainersFragment UIF_ItemContainers::GetItemContainersFragmentFromItem(FS_InventoryItem Item)
{
	UFL_InventoryFramework::UpdateItemStruct(Item);

	FItemContainersFragment* ItemContainersFragment = FindFragment<FItemContainersFragment>(Item.ItemFragments);
	return ItemContainersFragment ? *ItemContainersFragment : FItemContainersFragment();
}

FItemContainersFragment UIF_ItemContainers::GetItemContainersFragmentFromItemAsset(UDA_CoreItem* ItemAsset,
	bool GetFromAssetFragments)
{
	FItemContainersFragment* ItemContainersFragment = FindFragment<FItemContainersFragment>(
		GetFromAssetFragments ? ItemAsset->ItemAssetFragments : ItemAsset->ItemStructFragments);
	
	return ItemContainersFragment ? *ItemContainersFragment : FItemContainersFragment();
}

FItemContainersFragment UIF_ItemContainers::ToItemContainersFragment(FInstancedStruct Struct)
{
	if(const FItemContainersFragment* ItemContainersFragment = Struct.GetPtr<FItemContainersFragment>())
	{
		return *ItemContainersFragment;
	}

	return FItemContainersFragment();
}
