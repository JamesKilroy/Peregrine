// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Objects/Parents/O_TagValueCalculation.h"

float UO_TagValueCalculation::CalculateItemTagValue_Implementation(FS_TagValue TagValue, FS_InventoryItem Item)
{
	return TagValue.Value;
}

float UO_TagValueCalculation::CalculateContainerTagValue_Implementation(FS_TagValue TagValue,
	FS_ContainerSettings Container)
{
	return TagValue.Value;
}

bool UO_TagValueCalculation::ShouldValueBeRemovedFromItem_Implementation(FS_TagValue TagValue, FS_InventoryItem Item)
{
	return false;
}

bool UO_TagValueCalculation::ShouldValueBeRemovedFromContainer_Implementation(FS_TagValue TagValue, FS_ContainerSettings Container)
{
	return false;
}

float UO_TagValueCalculation::CalculateComponentTagValue_Implementation(FS_TagValue TagValue, UAC_Inventory* Component)
{
	return TagValue.Value;
}

bool UO_TagValueCalculation::ShouldValueBeRemovedFromComponent_Implementation(FS_TagValue TagValue,
	UAC_Inventory* Component)
{
	return false;
}
