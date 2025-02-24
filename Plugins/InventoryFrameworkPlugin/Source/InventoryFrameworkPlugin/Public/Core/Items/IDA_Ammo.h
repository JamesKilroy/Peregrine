// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Core/Items/DA_CoreItem.h"
#include "IDA_Ammo.generated.h"


/**Example class for ammo. Can safely be deleted for projects
 * that do not have any ammo items.*/

UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIDA_Ammo : public UDA_CoreItem
{
	GENERATED_BODY()

	virtual FText GetAssetTypeName() override;
};
