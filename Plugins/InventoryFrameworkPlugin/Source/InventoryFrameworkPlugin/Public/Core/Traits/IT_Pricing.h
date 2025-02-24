// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Items/IDA_Currency.h"
#include "ItemTrait.h"
#include "IT_Pricing.generated.h"

/**
 * 
 */
UCLASS(DisplayName = "Pricing")
class INVENTORYFRAMEWORKPLUGIN_API UIT_Pricing : public UItemTrait
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price Settings")
	float Price = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Price Settings")
	TArray<UIDA_Currency*> DefaultAcceptedCurrencies;

	virtual TArray<FString> VerifyData_Implementation(UDA_CoreItem* ItemAsset) override;

	
	virtual bool AllowMultipleCopiesInDataAsset_Implementation() override;
};
