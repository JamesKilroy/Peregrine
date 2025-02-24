// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Traits/ItemTrait.h"


bool UItemTrait::AddedToItemAsset_Implementation(UDA_CoreItem* ItemAsset)
{
	return true;
}

bool UItemTrait::AllowMultipleCopiesInDataAsset_Implementation()
{
	return false;
}