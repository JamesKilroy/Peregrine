// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Traits/IT_CustomShapeData.h"

#include "Core/Items/DA_CoreItem.h"

TArray<FString> UIT_CustomShapeData::VerifyData_Implementation(UDA_CoreItem* ItemAsset)
{
	Super::VerifyData_Implementation(ItemAsset);

	if(!ItemAsset)
	{
		return TArray<FString>({"Item Asset not valid, can't verify."});
	}
	
	TArray<FString> ErrorMessages;
	if(DisabledTiles.IsEmpty() && AnchorPoint == FIntPoint(0, 0))
	{
		ErrorMessages.Add("No settings added to Custom Shape object");
	}

	const TArray<FIntPoint> ItemShape = ItemAsset->GetItemsPureShape(Zero);
	if(!ItemShape.Contains(AnchorPoint))
	{
		ErrorMessages.Add("Anchor point must be a visible tile from the items pure shape.");
	}

	return ErrorMessages;
}
