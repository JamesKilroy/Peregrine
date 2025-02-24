// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "LootTableSystem/Components/AC_LootTable.h"

#include "Core/Components/AC_Inventory.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Interfaces/I_Inventory.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LootTableSystem/Objects/O_LootPool.h"


// Sets default values for this component's properties
UAC_LootTable::UAC_LootTable()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;
	
}

void UAC_LootTable::BeginPlay()
{
	Super::BeginPlay();

	if(!GetOwner()->HasAuthority())
	{
		//Loot tables can only be used by the server. We remove on the next tick
		//because the engine is doing a for loop that doesn't take into account
		//that a component is being destroyed during the loop
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
		{
			DestroyComponent();
		});
	}
}

UAC_Inventory* UAC_LootTable::GetInventoryComponent()
{
	if(!GetOwner()->Implements<UI_Inventory>())
	{
		UFL_InventoryFramework::LogIFPMessage(this, "Owner does not implement the I_Inventory interface, can't get inventory component - UAC_LootTable::GetInventoryComponent");
	}

	UAC_Inventory* Inventory = nullptr;
	II_Inventory::Execute_GetInventoryComponent(GetOwner(), Inventory);
	return Inventory;
}

FS_InventoryItem UAC_LootTable::GetParentItem()
{
	if(!ItemID.IsValid())
	{
		return FS_InventoryItem();
	}

	return ItemID.ParentComponent->GetItemByUniqueID(ItemID);
}

TArray<FS_ContainerSettings> UAC_LootTable::GetParentItemsContainers()
{
	FS_InventoryItem ParentItem = GetParentItem();
	if(!ParentItem.IsValid())
	{
		return TArray<FS_ContainerSettings>();
	}

	return ParentItem.UniqueID.ParentComponent->GetItemsChildrenContainers(ParentItem);
}

void UAC_LootTable::PreLoadAssets()
{
	for(auto& CurrentPool : LootPools)
	{
		CurrentPool->PreLoadAssets();
	}
}

void UAC_LootTable::PreInventoryInitialized_Implementation(UAC_Inventory* Inventory)
{
	for(auto& CurrentPool : LootPools)
	{
		CurrentPool->ProcessPreInitializationLoot(Inventory);
	}
}

void UAC_LootTable::PostInventoryInitialized_Implementation(UAC_Inventory* Inventory)
{
	for(auto& CurrentPool : LootPools)
	{
		CurrentPool->ProcessPostInitializationLoot(Inventory);
	}

	HasProcessedLootPools = true;

	if(DestroyAfterProcessing)
	{
		DestroyComponent();
	}
}