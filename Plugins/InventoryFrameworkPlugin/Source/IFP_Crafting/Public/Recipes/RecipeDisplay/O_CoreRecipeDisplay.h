// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Recipes/O_RecipeObject.h"
#include "UObject/Object.h"
#include "O_CoreRecipeDisplay.generated.h"

class UDA_CoreCraftingRecipe;

/**Validator for whether a recipe should be displayed
 * in the crafting menu.*/
UCLASS(Abstract, Blueprintable, meta=(ShowWorldContextPin), EditInlineNew, DefaultToInstanced, HideCategories = ("DoNotShow"))
class IFP_CRAFTING_API UO_CoreRecipeDisplay : public UO_RecipeObject
{
	GENERATED_BODY()

public:

	UFUNCTION(Category = "Requirements", BlueprintCallable, BlueprintNativeEvent)
	bool CheckRequirements(AActor* Actor, UDA_CoreCraftingRecipe* Recipe);
};
