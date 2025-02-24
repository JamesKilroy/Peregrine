// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Core/Items/DA_CoreItem.h"
#include "IDA_Misc.generated.h"

/**Base class that would not meet any other category.
 * In general, this refers to "junk" items that are meant
 * to be sold by the player.*/

UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIDA_Misc : public UDA_CoreItem
{
	GENERATED_BODY()

public:

	virtual FText GetAssetTypeName() override;
};
