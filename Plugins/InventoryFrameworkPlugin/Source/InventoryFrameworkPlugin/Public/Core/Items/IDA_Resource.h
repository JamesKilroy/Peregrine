// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Core/Items/DA_CoreItem.h"
#include "IDA_Resource.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIDA_Resource : public UDA_CoreItem
{
	GENERATED_BODY()

	virtual FText GetAssetTypeName() override;
	
};
