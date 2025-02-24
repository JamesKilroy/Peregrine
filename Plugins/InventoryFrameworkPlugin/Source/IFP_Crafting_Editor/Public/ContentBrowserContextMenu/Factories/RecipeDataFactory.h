// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Recipes/RecipeData/O_CoreRecipeData.h"
#include "RecipeDataFactory.generated.h"

/**
 * 
 */
UCLASS()
class IFP_CRAFTING_EDITOR_API URecipeDataFactory : public UFactory
{
	GENERATED_BODY()

public:

	URecipeDataFactory(const FObjectInitializer& ObjectInitializer);

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

private:

	UPROPERTY()
	TSubclassOf<UO_CoreRecipeData> ObjectClass;
};
