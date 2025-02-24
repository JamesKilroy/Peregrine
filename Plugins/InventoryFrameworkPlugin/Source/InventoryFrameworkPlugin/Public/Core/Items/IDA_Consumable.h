// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Core/Items/DA_CoreItem.h"
#include "IDA_Consumable.generated.h"

/**Base class for all items that can be consumed.
 * This class does not hold any logic, it rather allows you to
 * categorize what is and isn't a consumable item, such as an apple.*/

UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIDA_Consumable : public UDA_CoreItem
{
	GENERATED_BODY()

	virtual FText GetAssetTypeName() override;
};
