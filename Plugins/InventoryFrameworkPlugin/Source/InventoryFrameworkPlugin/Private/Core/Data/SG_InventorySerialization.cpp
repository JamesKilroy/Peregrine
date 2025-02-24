// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Data/SG_InventorySerialization.h"

#include "Core/Components/AC_Inventory.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Interfaces/I_Inventory.h"
#include "Core/Objects/Parents/ItemInstance.h"
#include "Kismet/KismetSystemLibrary.h"

void USG_InventorySerialization::SaveItemInstance(UItemInstance* ItemInstance)
{
	if(!IsValid(ItemInstance))
	{
		return;
	}

	//Prepare a record of the object
	FItemInstanceRecord ObjectRecord;
	FMemoryWriter MemoryWriter = FMemoryWriter(ObjectRecord.PropertyData, true);
	FSaveGameArchive SaveGameArchive = FSaveGameArchive(MemoryWriter);

	//Start serializing the record
	FS_InventoryItem ItemData = ItemInstance->GetItemData();
	ObjectRecord.ObjectClass = ItemInstance->GetClass();
	ObjectRecord.ContainerIndex = ItemData.ContainerIndex;
	ObjectRecord.ItemIndex = ItemData.ItemIndex;
	ItemInstance->Serialize(SaveGameArchive);
	ItemInstanceRecords.Add(ObjectRecord);
}

void USG_InventorySerialization::LoadItemInstance(FS_InventoryItem Item)
{
	if(!Item.IsValid())
	{
		return;
	}

	if(!UFL_InventoryFramework::AreItemDirectionsValid(Item.UniqueID, Item.ContainerIndex, Item.ItemIndex))
	{
		return;
	}
	
	for(auto& CurrentRecord : ItemInstanceRecords)
	{
		if(CurrentRecord.ContainerIndex == Item.ContainerIndex && CurrentRecord.ItemIndex == Item.ItemIndex)
		{
			UItemInstance* ItemInstance = Item.ItemInstance;
			FMemoryReader Reader = FMemoryReader(CurrentRecord.PropertyData, true);
			FSaveGameArchive Archive = FSaveGameArchive(Reader);
			ItemInstance->Serialize(Archive);
			
			break;
		}
	}
}

void USG_InventorySerialization::SaveContainersForActor(AActor* Actor)
{
	// if(!Actor)
	// {
	// 	return;
	// }
	//
	// if(!Actor->Implements<II_Inventory>())
	// {
	// 	return;
	// }
	//
	// UAC_Inventory* Inventory = nullptr;
	// II_Inventory::Execute_GetInventoryComponent(Actor, Inventory);
	// if(!Inventory)
	// {
	// 	UFL_InventoryFramework::LogIFPMessage(Actor, "Could not save actor, failed GetInventoryComponent");
	// }
	//
	// int32 ActorIndex = SavedActors.Find(Actor);
	// if(ActorIndex == INDEX_NONE)
	// {
	// 	SavedActors.Add(Actor);
	// 	ActorIndex = SavedActors.Num();
	// }

	// FContainerRecord ContainerRecord;
	// ContainerRecord.ActorName = Actor->GetName();
	//
	// FMemoryWriter MemoryWriter = FMemoryWriter(ContainerRecord.PropertyData, true);
	// FSaveGameArchive ContainerArchive = FSaveGameArchive(MemoryWriter);
}

void USG_InventorySerialization::LoadContainersForActor(AActor* Actor)
{
	// int32 ActorIndex = SavedActors.Find(Actor);
	// if(ActorIndex == INDEX_NONE)
	// {
	// 	UFL_InventoryFramework::LogIFPMessage(Actor, "Could not load containers for actor, it had not been saved in the past");
	// 	return;
	// }
}
