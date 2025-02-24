// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Core/Items/DA_CoreItem.h"
#include "IDA_Note.generated.h"

/**Example class for a note that the player can read.
 * Can safely be deleted for projects that do not have
 * any note items.*/

UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIDA_Note : public UDA_CoreItem
{
	GENERATED_BODY()

	virtual FText GetAssetTypeName() override;
	
	virtual bool CanItemTypeStack() override;
};
