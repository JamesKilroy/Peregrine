// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/ItemComponents/IT_ItemAbility.h"

#include "Core/ItemComponents/IC_ItemAbility.h"


bool UIT_ItemAbility::LooselyCheckCanActivateAbility_Implementation(FS_InventoryItem Item)
{
	return true;
}

TArray<FString> UIT_ItemAbility::VerifyData_Implementation(UDA_CoreItem* ItemAsset)
{
	TArray<FString> ErrorMessages = Super::VerifyData_Implementation(ItemAsset);

	if(ItemComponent)
	{
		if(!ItemComponent->IsChildOf(UIC_ItemAbility::StaticClass()))
		{
			ErrorMessages.Add("ItemComponent is not a child of IT_ItemAbility");
		}
	}
	
	return ErrorMessages;
}
