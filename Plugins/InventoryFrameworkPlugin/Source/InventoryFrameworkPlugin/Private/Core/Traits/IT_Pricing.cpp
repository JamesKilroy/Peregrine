// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Traits/IT_Pricing.h"

TArray<FString> UIT_Pricing::VerifyData_Implementation(UDA_CoreItem* ItemAsset)
{
	Super::VerifyData_Implementation(ItemAsset);

	TArray<FString> ErrorMessages;

	if(DefaultAcceptedCurrencies.IsEmpty())
	{
		ErrorMessages.Add("Item has no default currency");
	}

	if(Price < 0)
	{
		ErrorMessages.Add("Pricing is negative");
	}

	return ErrorMessages;
}


bool UIT_Pricing::AllowMultipleCopiesInDataAsset_Implementation()
{
	return Super::AllowMultipleCopiesInDataAsset_Implementation();
}

