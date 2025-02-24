// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Recipes/DataAssets/DA_CoreCraftingRecipe.h"
#include "CraftingAssetFactory.generated.h"

/**
 * 
 */
UCLASS()
class IFP_CRAFTING_EDITOR_API UCraftingAssetFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=DataAsset)
	TSubclassOf<UDA_CoreCraftingRecipe> DataAssetClass;

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
};
