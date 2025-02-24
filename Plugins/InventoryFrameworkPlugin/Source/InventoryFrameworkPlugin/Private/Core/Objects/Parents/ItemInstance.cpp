// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Objects/Parents/ItemInstance.h"

#include "Core/Components/AC_Inventory.h"
#include "Net/UnrealNetwork.h"

void UItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UItemInstance, ItemID);
}

void UItemInstance::StartPlay_Implementation()
{
	Super::StartPlay_Implementation();

	UFL_ExternalObjects::BroadcastItemInstanceSpawned(GetItemData(), this);
}

void UItemInstance::OnRep_ItemID(FS_UniqueID OldUniqueID)
{
	FS_InventoryItem ParentItem = GetItemData();
	if(ParentItem.IsValid())
	{
		ParentItem.UniqueID.ParentComponent->ContainerSettings[ParentItem.ContainerIndex].Items[ParentItem.ItemIndex].ItemInstance = this;
	}
	
	ItemIDUpdated(OldUniqueID);
}

void UItemInstance::ItemIDUpdated_Implementation(FS_UniqueID OldUniqueID)
{
}

UAC_Inventory* UItemInstance::GetInventoryComponent()
{
	return ItemID.ParentComponent;
}

FS_InventoryItem UItemInstance::GetItemData()
{
	if(UAC_Inventory* Inventory = GetInventoryComponent())
	{
		return Inventory->GetItemByUniqueID(ItemID);
	}

	return FS_InventoryItem();
}

FS_ContainerSettings UItemInstance::GetContainerSettings()
{
	if(UAC_Inventory* Inventory = GetInventoryComponent())
	{
		FS_InventoryItem ItemData = GetItemData();
		if(Inventory->ContainerSettings.IsValidIndex(ItemData.ContainerIndex))
		{
			return Inventory->ContainerSettings[ItemData.ContainerIndex];
		}
	}

	return FS_ContainerSettings();
}

void UItemInstance::RemoveObject()
{
	MarkAsGarbage();

	Super::RemoveObject();
}

AActor* UItemInstance::GetOwningActor() const
{
	if(ItemID.IsValid())
	{
		return ItemID.ParentComponent->GetOwner();
	}
	
	if(Owner)
	{
		return Owner;
	}

	return GetTypedOuter<AActor>();
}
