// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Fragments/FL_IFP_FragmentHelpers.h"

#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Data/IFP_CoreData.h"



TInstancedStruct<FCoreFragment> UFL_IFP_FragmentHelpers::GetItemFragmentByStruct(FS_InventoryItem Item,
	UScriptStruct* Struct)
{
	UFL_InventoryFramework::UpdateItemStruct(Item);
	
	for(auto& CurrentFragment : Item.ItemFragments)
	{
		if(CurrentFragment.GetScriptStruct() == Struct)
		{
			return CurrentFragment;
		}
	}
	
	return TInstancedStruct<FCoreFragment>();
}

TArray<int32> UFL_IFP_FragmentHelpers::FindDuplicateFragments(
	TArray<TInstancedStruct<FCoreFragment>> Fragments)
{
	TArray<int32> DuplicateIndices;

	for (int32 i = 0; i < Fragments.Num(); ++i)
	{
		for (int32 j = i + 1; j < Fragments.Num(); ++j)
		{
			if(Fragments[i].GetScriptStruct() == Fragments[j].GetScriptStruct())
			{
				DuplicateIndices.Add(j);
			}
		}
	}

	return DuplicateIndices;  // Return the indices of all duplicate fragments
}
