// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Core/Items/DA_CoreItem.h"
#include "IDA_ItemAttachment.generated.h"

/**Example class for item attachments, such as a sight.
 * Can safely be deleted for projects that do not have
 * any item attachments
 *
 * Any item can be attached onto another item, it all
 * depends on your compatibility settings. This simply
 * shares the same philosophy as the consumable class,
 * where you want to categorize items under a
 * specific class.*/

UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIDA_ItemAttachment : public UDA_CoreItem
{
	GENERATED_BODY()

public:

	virtual bool CanItemTypeStack() override;

	virtual FText GetAssetTypeName() override;
};
