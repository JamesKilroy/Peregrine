// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Fragments/IF_RandomItemCount.h"

#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Fragments/FL_IFP_FragmentHelpers.h"
#include "Core/Items/DA_CoreItem.h"

bool FRandomItemCountFragment::IsCompatibleWithItem(UObject* ItemOwner,
                                                    const FS_InventoryItem& Item)
{
	if(!Item.ItemAssetSoftReference.IsNull())
	{
		return Item.ItemAssetSoftReference.LoadSynchronous()->CanItemStack();
	}
	
	return FItemFragment::IsCompatibleWithItem(ItemOwner, Item);
}

FRandomItemCountFragment UIF_RandomItemCount::GetRandomItemCountFragmentFromItem(FS_InventoryItem Item)
{
	UFL_InventoryFramework::UpdateItemStruct(Item);

	FRandomItemCountFragment* RandomItemCountFragment = FindFragment<FRandomItemCountFragment>(Item.ItemFragments);
	return RandomItemCountFragment ? *RandomItemCountFragment : FRandomItemCountFragment();
}

FRandomItemCountFragment UIF_RandomItemCount::GetRandomItemCountFragmentFromItemAsset(UDA_CoreItem* ItemAsset,
	bool GetFromAssetFragments)
{
	FRandomItemCountFragment* RandomItemCountFragment = FindFragment<FRandomItemCountFragment>(
	GetFromAssetFragments ? ItemAsset->ItemAssetFragments : ItemAsset->ItemStructFragments);
	
	return RandomItemCountFragment ? *RandomItemCountFragment : FRandomItemCountFragment();
}

FRandomItemCountFragment UIF_RandomItemCount::ToRandomItemCountFragment(FInstancedStruct Struct)
{
	if(const FRandomItemCountFragment* TagFragment = Struct.GetPtr<FRandomItemCountFragment>())
	{
		return *TagFragment;
	}

	return FRandomItemCountFragment();
}
