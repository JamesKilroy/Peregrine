// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Recipes/O_RecipeObject.h"
#include "UObject/Object.h"
#include "O_CoreRecipeData.generated.h"

/**Container for optional data you want to associate
 * with a recipe, which you then retrieve in your code
 * to adapt your UI or code to use.
 * This can be used for time to craft, recipe
 * description, or some other data.*/
UCLASS(Abstract, Blueprintable, meta=(ShowWorldContextPin), EditInlineNew, DefaultToInstanced, HideCategories = ("DoNotShow"))
class IFP_CRAFTING_API UO_CoreRecipeData : public UO_RecipeObject
{
	GENERATED_BODY()

	//Meant to be empty, create children of this class
	//to add to a recipe's data objects.
};
