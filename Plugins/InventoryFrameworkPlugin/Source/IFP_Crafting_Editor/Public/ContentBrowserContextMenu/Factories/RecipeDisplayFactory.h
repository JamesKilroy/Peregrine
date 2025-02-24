// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Recipes/RecipeDisplay/O_CoreRecipeDisplay.h"
#include "RecipeDisplayFactory.generated.h"

/**
 * 
 */
UCLASS()
class IFP_CRAFTING_EDITOR_API URecipeDisplayFactory : public UFactory
{
	GENERATED_BODY()

public:

	URecipeDisplayFactory(const FObjectInitializer& ObjectInitializer);

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

private:

	UPROPERTY()
	TSubclassOf<UO_CoreRecipeDisplay> ObjectClass;
};
