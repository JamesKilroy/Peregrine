// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Fragments/CF_TileTags.h"

#include "Core/Fragments/FL_IFP_FragmentHelpers.h"

bool FTileTagsFragment::InitializeFragment(UAC_Inventory* InventoryComponent, const FS_ContainerSettings& Container,
	const FS_InventoryItem& Item)
{
	if(OptimizeForGameThread)
	{
		int32 MaxIndex = Container.Dimensions.X * Container.Dimensions.Y - TileTags.Num();
		TArray<FS_TileTag> NewTileTags;
		for(int32 i = 0; i <= MaxIndex; ++i)
		{
			NewTileTags.Add(FS_TileTag(i, FGameplayTagContainer()));
		}
		
		for(auto& CurrentTile : TileTags)
		{
			NewTileTags.Insert(CurrentTile, CurrentTile.TileIndex);
		}

		TileTags = NewTileTags;
	}
	
	return FContainerFragment::InitializeFragment(InventoryComponent, Container, Item);
}

bool FTileTagsFragment::IsCompatibleWithContainer(UAC_Inventory* InventoryComponent, const FS_ContainerSettings& Container)
{
	if(!Container.SupportsTileMap())
	{
		//This fragment requires a tilemap
		return false;
	}
	
	return FContainerFragment::IsCompatibleWithContainer(InventoryComponent, Container);
}

FTileTagsFragment UCF_TileTags::GetTileTagsFragmentFromContainer(FS_ContainerSettings Container)
{
	FTileTagsFragment* TileTagsFragment = FindFragment<FTileTagsFragment>(Container.ContainerFragments);
	return TileTagsFragment ? *TileTagsFragment : FTileTagsFragment();
}

FTileTagsFragment UCF_TileTags::ToTileTagsFragment(FInstancedStruct Struct)
{
	if(const FTileTagsFragment* TileTagsFragment = Struct.GetPtr<FTileTagsFragment>())
	{
		return *TileTagsFragment;
	}

	return FTileTagsFragment();
}

int32 UCF_TileTags::GetTileTagIndexForTile(FS_ContainerSettings Container, int32 TileIndex)
{
	FTileTagsFragment* TileTagsFragment = FindFragment<FTileTagsFragment>(Container.ContainerFragments);
	if(TileTagsFragment)
	{
		if(TileTagsFragment->OptimizeForGameThread)
		{
			if(TileTagsFragment->TileTags.IsValidIndex(TileIndex))
			{
				return TileIndex;
			}
		}
		else
		{
			for(int32 CurrentIndex = 0; CurrentIndex < TileTagsFragment->TileTags.Num(); CurrentIndex++)
			{
				if(TileTagsFragment->TileTags[CurrentIndex].TileIndex == TileIndex)
				{
					return CurrentIndex;
				}
			}
		}
	}

	return -1;
}

FGameplayTagContainer UCF_TileTags::GetTilesTags(FS_ContainerSettings Container, int32 TileIndex)
{
	FTileTagsFragment TileTagsFragment = GetTileTagsFragmentFromContainer(Container);
	int32 Index = GetTileTagIndexForTile(Container, TileIndex);
	if(TileTagsFragment.TileTags.IsValidIndex(Index))
	{
		return TileTagsFragment.TileTags[Index].Tags;
	}

	return FGameplayTagContainer();
}
