// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "Core/Items/DA_CoreItem.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "FL_IFP_FragmentHelpers.generated.h"

/**Template function to find a fragment by its type in an array.
 * Optionally can add the fragment if it's missing.*/
template <typename T>
T* FindFragment(TArray<TInstancedStruct<FCoreFragment>>& Array, bool AddIfMissing = false)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FindFragment)
	
	static_assert(TIsDerivedFrom<T, FCoreFragment>::IsDerived, "T must be derived from FCoreFragment");

	UScriptStruct* TargetStruct = T::StaticStruct();

	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Array)
	{
		if(FCoreFragment* Fragment = InstancedStruct.GetMutablePtr<>())
		{
			if(InstancedStruct.GetScriptStruct() == TargetStruct)
			{
				return reinterpret_cast<T*>(Fragment);
			}
		}
	}

	//If not found and AddIfMissing is true, create a new fragment and add it to the array
	if(AddIfMissing)
	{
		//New instances MUST have a set type to create.
		TInstancedStruct<FCoreFragment> NewInstancedStruct;
		//Initialize with type T so it becomes the fragment we want
		NewInstancedStruct.InitializeAs<T>(); 
		Array.Add(NewInstancedStruct);
		
		return Array.Last().GetMutablePtr<T>();
	}
	
	return nullptr;
}

template <typename T>
TInstancedStruct<T> FindTemplatedFragment(TArray<TInstancedStruct<FCoreFragment>>& Array)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FindTemplatedFragment)
	
	static_assert(TIsDerivedFrom<T, FCoreFragment>::IsDerived, "T must be derived from FCoreFragment");

	UScriptStruct* TargetStruct = T::StaticStruct();

	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Array)
	{
		if(FCoreFragment* Fragment = InstancedStruct.GetMutablePtr<>())
		{
			if(InstancedStruct.GetScriptStruct() == TargetStruct)
			{
				TInstancedStruct<T> TypedFragment;
				TypedFragment.template InitializeAs<T>(*reinterpret_cast<T*>(Fragment));
				return TypedFragment;
			}
		}
	}

	TInstancedStruct<T> NullFragment;
	return NullFragment;
}

/**Helper template function to remove a fragment from an array.
 * Since the fragment arrays are TArray<TInstancedStruct<FCoreFragment>>
 * but we rarely work with a TInstancedStruct<FCoreFragment>, it becomes
 * annoying to remove it. This function just takes in the pointer and
 * removes the fragment by its index. */ 
template <typename T>
bool RemoveFragmentFromArray(TArray<TInstancedStruct<FCoreFragment>>& Array, T* FragmentToRemove)
{
	if (!FragmentToRemove)
	{
		return false;
	}

	for (int32 Index = 0; Index < Array.Num(); ++Index)
	{
		if (Array[Index].GetScriptStruct() == T::StaticStruct())
		{
			Array.RemoveAtSwap(Index);
			return true; 
		}
	}

	return false;
}

/**Template function to find a fragment by its type in an array.
 * This will search both the struct fragments and the item asset fragments.*/
template <typename T>
T* FindFragmentFromItem(FS_InventoryItem& Item, bool& FoundInStruct)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FindFragmentFromItem)
	
	static_assert(TIsDerivedFrom<T, FCoreFragment>::IsDerived, "T must be derived from FCoreFragment");

	UScriptStruct* TargetStruct = T::StaticStruct();

	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Item.ItemFragments)
	{
		if(FCoreFragment* Fragment = InstancedStruct.GetMutablePtr<>())
		{
			if(InstancedStruct.GetScriptStruct() == TargetStruct)
			{
				FoundInStruct = true;
				return reinterpret_cast<T*>(Fragment);
			}
		}
	}

	for(TInstancedStruct<FCoreFragment>& InstancedStruct : Item.ItemAsset->ItemAssetFragments)
	{
		if(FCoreFragment* Fragment = InstancedStruct.GetMutablePtr<>())
		{
			if(InstancedStruct.GetScriptStruct() == TargetStruct)
			{
				FoundInStruct = false;
				return reinterpret_cast<T*>(Fragment);
			}
		}
	}

	FoundInStruct = false;
	return nullptr;
}


/**Base library for exposing C++ helper functions to Blueprint,
 * or common functions to help with fragments.*/
UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UFL_IFP_FragmentHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(Category = "IFP|Fragments", BlueprintCallable)
	static TInstancedStruct<FCoreFragment> GetItemFragmentByStruct(FS_InventoryItem Item, UScriptStruct* Struct);
	
	static TArray<int32> FindDuplicateFragments(TArray<TInstancedStruct<FCoreFragment>> Fragments);
};
