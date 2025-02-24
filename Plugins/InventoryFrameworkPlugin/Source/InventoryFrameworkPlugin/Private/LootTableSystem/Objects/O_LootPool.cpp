// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "LootTableSystem/Objects/O_LootPool.h"

#include "Core/Components/AC_Inventory.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Fragments/FL_IFP_FragmentHelpers.h"
#include "Core/Fragments/F_Tags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LootTableSystem/Components/AC_LootTable.h"
#include "LootTableSystem/Data/FL_LootTableHelpers.h"

UWorld* UO_LootPool::GetWorld() const
{
	if (IsTemplate() || !GetOuter()) // We're the CDO or have no outer (?!).
	{
		return nullptr;
	}
	return GetOuter()->GetWorld();
}

void UO_LootPool::StartPlay_Implementation()
{
}

#if WITH_EDITOR
void UO_LootPool::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UObject::PostEditChangeProperty(PropertyChangedEvent);

	PostPropertyChange(PropertyChangedEvent.GetPropertyName(), PropertyChangedEvent.GetMemberPropertyName());
}
#endif

int32 UO_LootPool::GetNumberOfItemsAdded(bool IncludeOtherLootTables)
{
	if(!IncludeOtherLootTables)
	{
		return ItemsSpawned;
	}

	int32 TotalItems = ItemsSpawned;
	TArray<UAC_LootTable*> LootTables = UFL_LootTableHelpers::GetLootTableComponents(GetLootTable()->GetOwner(), true);
	for(auto& CurrentTable : LootTables)
	{
		for(auto& CurrentPool : CurrentTable->LootPools)
		{
			if(CurrentPool == this)
			{
				continue;
			}
			
			TotalItems += CurrentPool->ItemsSpawned;
		}
	}

	return TotalItems;
}

UAC_Inventory* UO_LootPool::GetInventoryComponent()
{
	return GetLootTable()->GetInventoryComponent();
}

UAC_LootTable* UO_LootPool::GetLootTable()
{
	return Cast<UAC_LootTable>(GetOuter());
}

void UO_LootPool::AddItemPreInitializeOnly(FS_InventoryItem Item, FS_ContainerSettings Container, bool IncludeLootTable)
{
	if(!Container.IsValid())
	{
		return;
	}
	
	UAC_Inventory* Inventory = GetInventoryComponent();
	if(!Inventory)
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Couldn't add item, no inventory found - UO_LootPool::AddItemPreInitializeOnly");
		return;
	}

	if(IncludeLootTable)
	{
		if(FTagFragment* TagFragment = FindFragment<FTagFragment>(Item.ItemFragments, true))
		{
			TagFragment->Tags.AddTag(IFP_IncludeLootTables);
		}
	}

	Item.ContainerIndex = Container.ContainerIndex;
	Item.ItemIndex = Inventory->ContainerSettings[Container.ContainerIndex].Items.Num();
	Inventory->ContainerSettings[Container.ContainerIndex].Items.Add(Item);
	Inventory->QueuedLootTableItems.Add(Item);
}

void UO_LootPool::PreLoadAssets_Implementation()
{
}
