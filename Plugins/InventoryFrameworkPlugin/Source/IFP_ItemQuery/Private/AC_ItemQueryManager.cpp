// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "AC_ItemQueryManager.h"

#include "O_ItemQueryBase.h"
#include "Core/Components/AC_Inventory.h"


UAC_ItemQueryManager::UAC_ItemQueryManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UAC_ItemQueryManager::IncrementQueriesFinished()
{
	QueuedQueriesFinished++;
	if(QueuedQueriesFinished >= QueuedQueries)
	{
		QueriesRegistered.Broadcast();
		QueuedQueries = 0;
		QueuedQueriesFinished = 0;
	}
}

void UAC_ItemQueryManager::RegisterItemQuery(UO_ItemQueryBase* ItemQuery, bool DoNotRefresh, bool MultiThreadRefresh)
{
	//Query is already initialized
	if(ItemQuery->ItemQueryManager)
	{
		return;
	}

	//Query might be in the default queries array, don't add it again
	ItemQueries.AddUnique(ItemQuery);
	ItemQuery->ItemQueryManager = this;
	ItemQuery->Inventory = Inventory;
	QueuedQueries++;
	ItemQuery->QueryRegistered(this, Inventory, DoNotRefresh, MultiThreadRefresh);
}

void UAC_ItemQueryManager::RegisterItemQueryWithCallback(UO_ItemQueryBase* ItemQuery, FQueryRefreshCallback Callback)
{
	if(ItemQueries.Contains(ItemQuery))
	{
		return;
	}

	RegisterItemQuery(ItemQuery, true, true);
	ItemQuery->RefreshItemsWithCallback(false, Callback);
}

TArray<UO_ItemQueryBase*> UAC_ItemQueryManager::RegisterItemWithQueries(FS_InventoryItem Item)
{
	TArray<UO_ItemQueryBase*> Queries;
	for(auto& CurrentQuery : ItemQueries)
	{
		if(CurrentQuery->RegisterItem(Item))
		{
			Queries.Add(CurrentQuery);
		}
	}
	return Queries;
}

void UAC_ItemQueryManager::UnregisterItemWithQueries(FS_InventoryItem Item)
{
	for(auto& CurrentQuery : ItemQueries)
	{
		CurrentQuery->UnregisterItem(Item);
	}
}

TArray<FS_InventoryItem> UAC_ItemQueryManager::GetItemsFromQueryByClass(TSubclassOf<UO_ItemQueryBase> QueryClass, bool UpdateCachedItems, bool SortByContainerAndItemIndex)
{
	for(auto& CurrentQuery : ItemQueries)
	{
		if(CurrentQuery->GetClass() == QueryClass)
		{
			return CurrentQuery->GetItemsFromQuery(UpdateCachedItems, SortByContainerAndItemIndex);
		}
	}

	return TArray<FS_InventoryItem>();
}

TArray<FS_InventoryItem> UAC_ItemQueryManager::GetItemsFromQueryByIdentifier(FGameplayTag Identifier, bool UpdateCachedItems,
	bool SortByContainerAndItemIndex)
{
	for(auto& CurrentQuery : ItemQueries)
	{
		if(CurrentQuery->Identifier == Identifier)
		{
			return CurrentQuery->GetItemsFromQuery(UpdateCachedItems, SortByContainerAndItemIndex);
		}
	}

	return TArray<FS_InventoryItem>();
}

void UAC_ItemQueryManager::GetQueriesByClass(TSubclassOf<UO_ItemQueryBase> QueryClass,
	TArray<UO_ItemQueryBase*>& Queries)
{
	for(auto& CurrentQuery : ItemQueries)
	{
		if(CurrentQuery->GetClass() == QueryClass)
		{
			Queries.Add(CurrentQuery);
		}
	}
}

TArray<UO_ItemQueryBase*> UAC_ItemQueryManager::GetQueriesByIdentifier(FGameplayTag Identifier)
{
	TArray<UO_ItemQueryBase*> Queries;
	
	for(auto& CurrentQuery : ItemQueries)
	{
		if(CurrentQuery->Identifier == Identifier)
		{
			Queries.Add(CurrentQuery);
		}
	}

	return Queries;
}

// Called when the game starts
void UAC_ItemQueryManager::BeginPlay()
{
	Super::BeginPlay();

	// ...

	if(AActor* OwningActor = GetOwner())
	{
		//Find, then cache the pointer to the inventory
		Inventory = Cast<UAC_Inventory>(OwningActor->GetComponentByClass(UAC_Inventory::StaticClass()));
		if(Inventory)
		{
			for(auto& CurrentQuery : ItemQueries)
			{
				if(!CurrentQuery)
				{
					continue;
				}
				
				RegisterItemQuery(CurrentQuery, false, true);
			}
		}
	}
}





