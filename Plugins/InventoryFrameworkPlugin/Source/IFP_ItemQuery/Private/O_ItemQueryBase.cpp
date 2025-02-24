// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "O_ItemQueryBase.h"
#include "Async/Async.h"
#include "AC_ItemQueryManager.h"
#include "Core/Data/FL_InventoryFramework.h"

UWorld* UO_ItemQueryBase::GetWorld() const
{
	if (IsTemplate() || !GetOuter())
	{
		return nullptr;
	}
	return GetOuter()->GetWorld();
}

void UO_ItemQueryBase::QueryRegistered(UAC_ItemQueryManager* QueryManager, UAC_Inventory* InventoryComponent, bool DoNotRefresh, bool MultiThreadRefresh)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(QueryRegistered)
	Inventory = InventoryComponent;

	//Assign the four most important delegates
	Inventory->StartMultithreadWork.AddDynamic(this, &UO_ItemQueryBase::OnInventoryStarted);
	Inventory->ComponentStopped.AddDynamic(this, &UO_ItemQueryBase::OnInventoryStopped);
	Inventory->ItemAdded.AddDynamic(this, &UO_ItemQueryBase::OnItemAdded);
	Inventory->ItemRemoved.AddDynamic(this, &UO_ItemQueryBase::OnItemRemoved);
	
	if(Inventory->Initialized && !DoNotRefresh)
	{
		/**Since we have just registered and the component is initialized,
		 * we can attempt to query all items to see if they fit in our filter.*/
		RefreshItems(MultiThreadRefresh);
	}

	/**Call the Blueprint event. I do this just because people on the
	 * marketplace have on occasion forgotten to call the parent function
	 * and we need the C++ implementation of QueryRegistered to be called.*/
	K2_QueryRegistered(QueryManager, InventoryComponent);

	if(QueryManager && !MultiThreadRefresh)
	{
		QueryManager->IncrementQueriesFinished();
	}
}

bool UO_ItemQueryBase::RegisterItem(FS_InventoryItem Item)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(RegisterItem)
	if(!Item.IsValid())
	{
		return false;
	}

	/**Technically it is safest to update the item struct before
	 * registering it. BUT for multithreading, this was very
	 * randomly causing a crash.
	 * Either way, you should already be attempting to register
	 * the most up to date item struct AND this item will get
	 * updated anyways when you call GetItemsFromQuery with
	 * UpdateCachedItems enabled.*/
	// UFL_InventoryFramework::UpdateItemStruct(Item);

	if(Item.ParentComponent() != Inventory)
	{
		//Item is not in our inventory component, not supported behavior
		return false;
	}

	if(!UFL_InventoryFramework::AreItemDirectionsValid(Item.UniqueID, Item.ContainerIndex, Item.ItemIndex))
	{
		return false;
	}

	if(RegisteredItems.Contains(Item))
	{
		return false;
	}
	
	if(!DoesItemPassFilter(Item))
	{
		return false;
	}

	RegisteredItems.Add(Item);
	ItemRegistered(Item);
	return true;
}

bool UO_ItemQueryBase::UnregisterItem(FS_InventoryItem Item)
{
	if(RegisteredItems.Contains(Item))
	{
		RegisteredItems.RemoveSingle(Item);
		ItemUnregistered(Item);
		return true;
	}

	return false;
}

void UO_ItemQueryBase::RefreshItems(bool MultiThreadRefresh, bool OnlyRefreshRegisteredItems)
{
	if(MultiThreadRefresh)
	{
		FQueryRefreshCallback Callback;
		RefreshItemsWithCallback(OnlyRefreshRegisteredItems, Callback);
	}
	else
	{
		if(OnlyRefreshRegisteredItems)
		{
			//Make a copy of the RegisteredItems array,
			//then clear it and attempt to register the items
			TArray<FS_InventoryItem> TemporaryItems = RegisteredItems;
			RegisteredItems.Empty();
			for(auto& CurrentItem : TemporaryItems)
			{
				RegisterItem(CurrentItem);
			}
		}
		else
		{
			RegisteredItems.Empty();
			for(auto& CurrentContainer : Inventory->ContainerSettings)
			{
				for(auto& CurrentItem : CurrentContainer.Items)
				{
					RegisterItem(CurrentItem);
				}
			}
		}

		if(ItemQueryManager.Get())
		{
			ItemQueryManager->IncrementQueriesFinished();
		}

		QueryRefreshed.Broadcast();
	}
}

void UO_ItemQueryBase::RefreshItemsWithCallback(bool OnlyRefreshRegisteredItems, FQueryRefreshCallback Callback)
{
	//Make a copy for safety.
	TArray<FS_ContainerSettings> Containers = Inventory->ContainerSettings;
	//I've found that AsyncTask is safer when using TObjectPtr
	TObjectPtr<UO_ItemQueryBase> QueryReference = this;
	
	/**V: Technically AnyThread is bad because it can land on a background thread.
	 * We want this to run on any available FOREGROUND thread whenever possible,
	 * but that doesn't seem doable without going into much more complex
	 * multithreading code.
	 * For now I am leaving it to AnyThread, since its the only option that I've
	 * found that runs the task on a foreground thread. Every other option would
	 * go onto the RHI thread. If there are any random ominous crashes, might be
	 * worthwhile seeing if this is the issue.*/
	AsyncTask(ENamedThreads::AnyThread, [QueryReference, OnlyRefreshRegisteredItems, Containers, Callback]()
	{
		if(!QueryReference.Get() || !QueryReference->Inventory.Get())
		{
			//Either this or the inventory got destroyed by the time
			//this thread got the task
			return;
		}
		
		if(OnlyRefreshRegisteredItems)
		{
			//Make a copy of the RegisteredItems array,
			//then clear it and attempt to register the items
			TArray<FS_InventoryItem> TemporaryItems = QueryReference->RegisteredItems;
			QueryReference->RegisteredItems.Empty();
			for(auto& CurrentItem : TemporaryItems)
			{
				QueryReference->RegisterItem(CurrentItem);
			}
		}
		else
		{
			QueryReference->RegisteredItems.Empty();
			for(auto& CurrentContainer : Containers)
			{
				if(!QueryReference.Get() || !QueryReference->Inventory.Get())
				{
					/**Sanity check, since we are on a different thread,
					 * the inventory might get destroyed on the game thread
					 * while we are registering items.
					 * If so, then then end this task*/
					return;
				}
			
				for(auto& CurrentItem : CurrentContainer.Items)
				{
					QueryReference->RegisterItem(CurrentItem);
				}
			}
		}

		//Go back to the game thread
		AsyncTask(ENamedThreads::GameThread, [QueryReference, Callback]()
		{
			if(QueryReference->ItemQueryManager.Get())
			{
				QueryReference->ItemQueryManager->IncrementQueriesFinished();
			}
			QueryReference->QueryRefreshed.Broadcast();
			Callback.ExecuteIfBound();
		});
	});
}

TArray<FS_InventoryItem> UO_ItemQueryBase::GetItemsFromQuery(bool UpdateCachedItems, bool SortByContainerAndItemIndex)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(GetItemsFromQuery)
	if(!UpdateCachedItems)
	{
		return RegisteredItems;
	}

	TArray<FS_InventoryItem> Items;
	for(auto& CurrentItem : RegisteredItems)
	{
		UFL_InventoryFramework::UpdateItemStruct(CurrentItem);
		Items.Add(CurrentItem);
	}

	if(SortByContainerAndItemIndex)
	{
		Items = UFL_InventoryFramework::SortItemsByContainerAndIndex(Items);
	}
	
	return Items;
}

bool UO_ItemQueryBase::DoesItemPassFilter_Implementation(FS_InventoryItem Item)
{
	return false;
}

void UO_ItemQueryBase::OnInventoryStarted()
{
	RefreshItems(true);
}

void UO_ItemQueryBase::OnInventoryStopped()
{
	for(auto& CurrentContainer : Inventory->ContainerSettings)
	{
		for(auto& CurrentItem : CurrentContainer.Items)
		{
			UnregisterItem(CurrentItem);
		}
	}
}

void UO_ItemQueryBase::OnItemAdded(FS_InventoryItem Item, int32 ToIndex, FS_ContainerSettings ToContainer)
{
	RegisterItem(Item);
}

void UO_ItemQueryBase::OnItemRemoved(FS_InventoryItem Item, FS_ContainerSettings FromContainer)
{
	UnregisterItem(Item);
}