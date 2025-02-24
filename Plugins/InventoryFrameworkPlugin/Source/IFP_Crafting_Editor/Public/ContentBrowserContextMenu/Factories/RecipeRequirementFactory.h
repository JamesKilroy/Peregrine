// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Recipes/RecipeRequirements/O_CoreRecipeRequirement.h"
#include "RecipeRequirementFactory.generated.h"

/**
 * 
 */
UCLASS()
class IFP_CRAFTING_EDITOR_API URecipeRequirementFactory : public UFactory
{
	GENERATED_BODY()

public:

	URecipeRequirementFactory(const FObjectInitializer& ObjectInitializer);

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

private:

	UPROPERTY()
	TSubclassOf<UO_CoreRecipeRequirement> ObjectClass;
};
