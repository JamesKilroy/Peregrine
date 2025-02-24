// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Widgets/W_AttachmentParent.h"

#include "Core/Data/FL_InventoryFramework.h"


void UW_AttachmentParent::GetContainers_Implementation(TArray<UW_Container*>& Containers)
{
	Containers = WidgetContainers;
}

void UW_AttachmentParent::GetOwningItemData(FS_InventoryItem& ItemData)
{
	ItemData = FS_InventoryItem();
	if(!ParentItemID.IsValid())
	{
		return;
	}

	ItemData = ParentItemID.ParentComponent->GetItemByUniqueID(ParentItemID);
}

void UW_AttachmentParent::GetInventory(UAC_Inventory*& Component)
{
	Component = ParentItemID.ParentComponent;
}
