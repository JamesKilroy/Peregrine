// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "O_ItemAssetValidation.generated.h"

class UDA_CoreItem;
/**
 * 
 */
UCLASS(Abstract, Blueprintable)
class INVENTORYFRAMEWORKPLUGIN_API UO_ItemAssetValidation : public UObject
{
	GENERATED_BODY()

public:
	
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Verification", meta = (DevelopmentOnly))
	bool VerifyData(const UDA_CoreItem* ItemAsset, TArray<FString>& ErrorMessages);
};
