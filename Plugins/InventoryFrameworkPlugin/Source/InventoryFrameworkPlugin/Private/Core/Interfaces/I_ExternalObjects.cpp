// Copyright (C) Varian Daemon 2023

#include "Core/Interfaces/I_ExternalObjects.h"

#include "Core/Components/AC_Inventory.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Fragments/IF_ItemOverrideSettings.h"

void UFL_ExternalObjects::BroadcastItemCountUpdated(FS_InventoryItem Item, int32 OldValue, int32 NewValue)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	//Some data may be stale, fetch a fresh copy
	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);
	
	if(ParentComponent->ContainerSettings.IsValidIndex(Item.ContainerIndex))
	{
		ParentComponent->ItemCountUpdated.Broadcast(Item, OldValue, NewValue);
	}

	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_ItemCountUpdated(CurrentObject, Item, OldValue, NewValue);
		}
	}
}

void UFL_ExternalObjects::BroadcastLocationUpdated(FS_InventoryItem Item)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	//Some data may be stale, fetch a fresh copy
	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);

	//We only send the interface events here, not broadcast the delegate because it requires
	//much more information.
	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_LocationUpdated(CurrentObject, Item);
		}
	}
}

void UFL_ExternalObjects::BroadcastSizeUpdated(FS_InventoryItem Item)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	//Some data may be stale, fetch a fresh copy
	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);
	
	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_SizeUpdated(CurrentObject, Item);
		}
	}
}

void UFL_ExternalObjects::BroadcastRotationUpdated(FS_InventoryItem Item)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	//Some data may be stale, fetch a fresh copy
	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);

	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_RotationUpdated(CurrentObject, Item);
		}
	}
}

void UFL_ExternalObjects::BroadcastRarityUpdated(FS_InventoryItem Item)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	//Some data may be stale, fetch a fresh copy
	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);

	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_RarityUpdated(CurrentObject, Item);
		}
	}
}

void UFL_ExternalObjects::BroadcastImageUpdated(FS_InventoryItem Item)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	//Some data may be stale, fetch a fresh copy
	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);

	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_ImageUpdated(CurrentObject, Item);
		}
	}
}

void UFL_ExternalObjects::BroadcastNameUpdated(FS_InventoryItem Item)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	//Some data may be stale, fetch a fresh copy
	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);

	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_NameUpdated(CurrentObject, Item);
		}
	}
}

void UFL_ExternalObjects::BroadcastIconUpdated(UTexture* NewTexture, FS_InventoryItem Item)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	//Some data may be stale, fetch a fresh copy
	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);

	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_IconUpdated(CurrentObject, NewTexture, Item);
		}
	}
}

void UFL_ExternalObjects::BroadcastAffordabilityUpdated(bool CanAfford, FS_InventoryItem Item)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	//Some data may be stale, fetch a fresh copy
	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);
	
	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_AffordabilityUpdated(CurrentObject, CanAfford, Item);
		}
	}
}

void UFL_ExternalObjects::BroadcastTagsUpdated(FGameplayTag Tag, bool Added, FS_InventoryItem Item,
	FS_ContainerSettings Container)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent ? Item.UniqueID.ParentComponent : Container.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}

	if(Item.IsValid())
	{
		//Some data may be stale, fetch a fresh copy
		Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);

		if(Added)
		{
			ParentComponent->ItemTagAdded.Broadcast(Item, Tag);
		}
		else
		{
			ParentComponent->ItemTagRemoved.Broadcast(Item, Tag);
		}
	
		for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
		{
			if(!CurrentObject)
			{
				continue;
			}
		
			if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
			{
				II_ExternalObjects::Execute_TagsUpdated(CurrentObject, Tag, Added, Item, FS_ContainerSettings());
			}
		}
	}
	else if(Container.IsValid())
	{
		//Some data may be stale, fetch a fresh copy
		Container = ParentComponent->GetContainerByUniqueID(Container.UniqueID);
		
		if(Added)
		{
			ParentComponent->ContainerTagAdded.Broadcast(Container, Tag);
		}
		else
		{
			ParentComponent->ContainerTagRemoved.Broadcast(Container, Tag);
		}
	
		for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForContainerBroadcast(Container))
		{
			if(!CurrentObject)
			{
				continue;
			}
		
			if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
			{
				II_ExternalObjects::Execute_TagsUpdated(CurrentObject, Tag, Added, FS_InventoryItem(), Container);
			}
		}
	}
}

void UFL_ExternalObjects::BroadcastTagValueUpdated(FS_TagValue TagValue, bool Added, float Delta, FS_InventoryItem Item,
	FS_ContainerSettings Container)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent ? Item.UniqueID.ParentComponent : Container.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}

	if(Item.IsValid())
	{
		//Some data may be stale, fetch a fresh copy
		Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);
	
		for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
		{
			if(!CurrentObject)
			{
				continue;
			}
		
			if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
			{
				II_ExternalObjects::Execute_TagValueUpdated(CurrentObject, TagValue, Added, Delta, Item, FS_ContainerSettings());
			}
		}
	}
	else if(Container.IsValid())
	{
		//Some data may be stale, fetch a fresh copy
		Container = ParentComponent->GetContainerByUniqueID(Container.UniqueID);
	
		for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForContainerBroadcast(Container))
		{
			if(!CurrentObject)
			{
				continue;
			}
		
			if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
			{
				II_ExternalObjects::Execute_TagValueUpdated(CurrentObject, TagValue, Added, Delta, FS_InventoryItem(), Container);
			}
		}
	}
}

void UFL_ExternalObjects::BroadcastOverrideSettingsUpdated(FS_InventoryItem Item,
	FItemOverrideSettings OldOverride, FItemOverrideSettings NewOverride)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	//Some data may be stale, fetch a fresh copy
	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);
	
	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_OverrideSettingsUpdated(CurrentObject, Item, OldOverride, NewOverride);
		}
	}
}

void UFL_ExternalObjects::BroadcastBackgroundColorUpdated(FS_InventoryItem Item, FLinearColor NewColor, bool IsTemporary)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	//Some data may be stale, fetch a fresh copy
	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);
	
	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_BackgroundColorUpdated(CurrentObject, Item, NewColor, IsTemporary);
		}
	}
}

void UFL_ExternalObjects::BroadcastWidgetSelectionUpdated(FS_InventoryItem Item, bool IsSelected)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	//Some data may be stale, fetch a fresh copy
	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);
	
	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_WidgetSelectionUpdated(CurrentObject, Item, IsSelected);
		}
	}
}

void UFL_ExternalObjects::BroadcastItemEquipStatusUpdate(FS_InventoryItem Item, bool Equipped, TArray<FName> CustomTriggerFilters, FS_InventoryItem OldItemData)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	if(OldItemData.IsValid())
	{
		ParentComponent = OldItemData.UniqueID.ParentComponent;
	}

	/**Apply the equip tag*/
	if(ParentComponent->EquipTag.IsValid())
	{
		if(Equipped)
		{
			ParentComponent->Internal_AddTagToItem(Item, ParentComponent->EquipTag);
		}
		else
		{
			ParentComponent->Internal_RemoveTagFromItem(Item, ParentComponent->EquipTag);
		}
	}
	
	if(Equipped)
	{
		ParentComponent->ItemEquipped.Broadcast(Item, CustomTriggerFilters);
	}
	else
	{
		if(OldItemData.IsValid())
		{
			OldItemData.UniqueID.ParentComponent->ItemUnequipped.Broadcast(Item, OldItemData, CustomTriggerFilters);
		}
		else
		{
			ParentComponent->ItemUnequipped.Broadcast(Item, OldItemData, CustomTriggerFilters);
		}
	}

	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_ItemEquipStatusUpdate(CurrentObject, Item, Equipped, OldItemData);
		}
	}

	FS_ContainerSettings ParentContainer = UFL_InventoryFramework::GetItemsParentContainer(Item);
	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForContainerBroadcast(ParentContainer))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_ItemEquipStatusUpdate(CurrentObject, Item, Equipped, OldItemData);
		}
	}
}

void UFL_ExternalObjects::BroadcastItemInstanceSpawned(FS_InventoryItem Item, UItemInstance* ItemInstance)
{
	UAC_Inventory* ParentComponent = Item.UniqueID.ParentComponent;
	if(!ParentComponent)
	{
		return;
	}
	
	//Some data may be stale, fetch a fresh copy
	Item = ParentComponent->GetItemByUniqueID(Item.UniqueID);
	
	for(auto& CurrentObject : UFL_InventoryFramework::GetObjectsForItemBroadcast(Item))
	{
		if(!CurrentObject)
		{
			continue;
		}
		
		if(CurrentObject->GetClass()->ImplementsInterface(UI_ExternalObjects::StaticClass()))
		{
			II_ExternalObjects::Execute_ItemInstanceSpawned(CurrentObject, Item, ItemInstance);
		}
	}
}

