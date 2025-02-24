// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Core/Items/DA_CoreItem.h"
#include "IDA_CraftingMaterial.generated.h"

/**Example class for crafting materials. Can safely be deleted for projects
 * that do not have any crafting systems.*/

UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIDA_CraftingMaterial : public UDA_CoreItem
{
	GENERATED_BODY()

	virtual FText GetAssetTypeName() override;
};
