// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Recipes/O_RecipeObject.h"
#include "UObject/Object.h"
#include "O_CoreRecipeRequirement.generated.h"

/**Validator for whether a recipe can or can not be crafted.*/
UCLASS(Abstract, Blueprintable, meta=(ShowWorldContextPin), EditInlineNew, DefaultToInstanced, HideCategories = ("DoNotShow"))
class IFP_CRAFTING_API UO_CoreRecipeRequirement : public UO_RecipeObject
{
	GENERATED_BODY()

public:

	UFUNCTION(Category = "Requirements", BlueprintCallable, BlueprintNativeEvent)
	bool CheckRequirements(AActor* Actor, UDA_CoreCraftingRecipe* Recipe);

	/**Helper function for other functions to find out what this recipe requirement
	 * has added to the recipe's required item ingredients and their quantities.
	 * This is not needed. An example can be found in RR_ItemIngredients.*/
	UFUNCTION(Category = "Requirements", BlueprintCallable, BlueprintNativeEvent)
	TMap<TSoftObjectPtr<UDA_CoreItem>, int32> GetItemIngredients();
};

