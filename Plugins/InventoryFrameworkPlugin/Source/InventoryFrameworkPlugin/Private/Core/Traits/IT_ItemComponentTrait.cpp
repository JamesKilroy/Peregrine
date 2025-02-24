// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Traits/IT_ItemComponentTrait.h"

#include "Core/Components/ItemComponent.h"


UIT_ItemComponentTrait::UIT_ItemComponentTrait()
{
}

TArray<FString> UIT_ItemComponentTrait::VerifyData_Implementation(UDA_CoreItem* ItemAsset)
{
	TArray<FString> ErrorMessages = Super::VerifyData_Implementation(ItemAsset);

	if(!ItemComponent)
	{
		ErrorMessages.Add("No item component assigned");
	}

	return ErrorMessages;
}
