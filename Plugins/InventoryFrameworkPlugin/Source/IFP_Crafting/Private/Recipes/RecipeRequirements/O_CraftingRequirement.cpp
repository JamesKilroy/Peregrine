// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Items/DA_CoreItem.h"
#include "Recipes/RecipeRequirements/O_CoreRecipeRequirement.h"

bool UO_CoreRecipeRequirement::CheckRequirements_Implementation(AActor* Actor, UDA_CoreCraftingRecipe* Recipe)
{
	return false;
}

TMap<TSoftObjectPtr<UDA_CoreItem>, int32> UO_CoreRecipeRequirement::GetItemIngredients_Implementation()
{
	TMap<TSoftObjectPtr<UDA_CoreItem>, int32> EmptyMap;
	return EmptyMap;
}