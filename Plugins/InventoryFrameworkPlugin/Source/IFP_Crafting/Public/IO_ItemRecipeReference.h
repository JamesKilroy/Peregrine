// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Traits/ItemTrait.h"
#include "Recipes/DataAssets/DA_CoreCraftingRecipe.h"
#include "IO_ItemRecipeReference.generated.h"


/**Simple trait to store references to any crafting recipes
 * that are related to this item. This is NOT needed for the
 * crafting system to work, it is simply a way for you to
 * quickly get any recipe's related to this item without
 * going through the entire recipe data base.*/
UCLASS(DisplayName = "Item Recipes")
class IFP_CRAFTING_API UIO_ItemRecipeReference : public UItemTrait
{
	GENERATED_BODY()

public:

	UPROPERTY(Category = "Settings", BlueprintReadOnly, EditAnywhere)
	TArray<TSoftObjectPtr<UDA_CoreCraftingRecipe>> CraftingRecipes;
};
