// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Core/Items/DA_CoreItem.h"
#include "IDA_Currency.generated.h"

/**Example class for currency. Can safely be deleted for projects
 * that do not want to have their currency interact with the inventory
 * system.
 *
 * In some games, currency is a "item", then either stored in a special
 * container or is "consumed" once the player interacts with it and
 * increases the "Money" counter/attribute to make it look like the player
 * has collected some currency.*/

UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIDA_Currency : public UDA_CoreItem
{
	GENERATED_BODY()
	
public:

	virtual FText GetAssetTypeName() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Currency Settings")
	TEnumAsByte<ECurrencyTypes> CurrencyType = DefaultCurrency;
};
