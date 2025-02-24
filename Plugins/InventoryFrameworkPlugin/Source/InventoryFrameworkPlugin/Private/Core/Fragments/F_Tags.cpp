// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Fragments/F_Tags.h"

#include "GameplayTagsManager.h"
#include "Core/Components/AC_Inventory.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Fragments/FL_IFP_FragmentHelpers.h"
#include "StructUtils/InstancedStruct.h"
#include "Core/Items/DA_CoreItem.h"

FTagFragment UF_Tags::GetTagFragmentFromItem(FS_InventoryItem Item)
{
	FTagFragment* TagFragment = FindFragment<FTagFragment>(Item.ItemFragments);
	return TagFragment ? *TagFragment : FTagFragment();
}

FTagFragment UF_Tags::GetTagFragmentFromItemAsset(UDA_CoreItem* ItemAsset, bool GetFromAssetFragments)
{
	FTagFragment* TagFragment = FindFragment<FTagFragment>(
		GetFromAssetFragments ? ItemAsset->ItemAssetFragments : ItemAsset->ItemStructFragments);
	
	return TagFragment ? *TagFragment : FTagFragment();
}

FTagFragment UF_Tags::GetTagFragmentFromContainer(FS_ContainerSettings Container)
{
	FTagFragment* TagFragment = FindFragment<FTagFragment>(Container.ContainerFragments);
	return TagFragment ? *TagFragment : FTagFragment();
}

FTagFragment UF_Tags::ToTagFragment(FInstancedStruct Struct)
{
	if(const FTagFragment* TagFragment = Struct.GetPtr<FTagFragment>())
	{
		return *TagFragment;
	}

	return FTagFragment();
}

void UF_Tags::SimpleAddTagsToItemsTagFragment(FS_InventoryItem Item, FGameplayTagContainer Tags)
{
	if(!UFL_InventoryFramework::AreItemDirectionsValid(Item.UniqueID, Item.ContainerIndex, Item.ItemIndex))
	{
		return;
	}

	/**Remember to fetch a ref to the actual item you are working with.
	 * The @Item parameter is a COPY, not a reference.*/
	FTagFragment* TagFragment = FindFragment<FTagFragment>(Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ItemFragments, true);
	if(TagFragment)
	{
		TagFragment->Tags.AppendTags(Tags);
	}
}

void UF_Tags::SimpleRemoveTagsFromItemsTagFragment(FS_InventoryItem Item, FGameplayTagContainer Tags)
{
	if(!UFL_InventoryFramework::AreItemDirectionsValid(Item.UniqueID, Item.ContainerIndex, Item.ItemIndex))
	{
		return;
	}

	/**Remember to fetch a ref to the actual item you are working with.
	 * The @Item parameter is a COPY, not a reference.*/
	FTagFragment* TagFragment = FindFragment<FTagFragment>(Item.UniqueID.ParentComponent->ContainerSettings[Item.ContainerIndex].Items[Item.ItemIndex].ItemFragments, true);
	if(TagFragment)
	{
		TagFragment->Tags.RemoveTags(Tags);
	}
}
