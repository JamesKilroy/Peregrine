// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "O_CoreRecipeData.h"
#include "RD_TimeToCraftBase.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class IFP_CRAFTING_API URD_TimeToCraftBase : public UO_CoreRecipeData
{
	GENERATED_BODY()

public:

	UFUNCTION(Category = "Time to Craft", BlueprintCallable, BlueprintNativeEvent)
	float GetTimeToCraft();
};
