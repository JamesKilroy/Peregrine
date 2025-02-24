// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Core/Items/DA_CoreItem.h"
#include "IDA_QuestItem.generated.h"

/**Example class for a quest item. Can safely be deleted for projects
 * that do not have any quest system.*/

UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIDA_QuestItem : public UDA_CoreItem
{
	GENERATED_BODY()

	virtual FText GetAssetTypeName() override;
};
