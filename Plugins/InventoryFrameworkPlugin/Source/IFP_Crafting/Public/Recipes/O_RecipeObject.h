// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "O_RecipeObject.generated.h"

struct FS_InventoryItem;
class UDA_CoreCraftingRecipe;
/**
 * 
 */
UCLASS()
class IFP_CRAFTING_API UO_RecipeObject : public UObject
{
	GENERATED_BODY()

public:

	/**Called before a recipe creates an item.
	 * This is where you might want to remove items from
	 * the players inventory to make space for the item
	 * that is about to be created.*/
	UFUNCTION(Category = "Events", BlueprintCallable, BlueprintImplementableEvent)
	void PreRecipeCrafted(AActor* Actor, UDA_CoreCraftingRecipe* Recipe);

	/**Called after a recipe has successfully created an item.*/
	UFUNCTION(Category = "Events", BlueprintCallable, BlueprintImplementableEvent)
	void PostRecipeCrafted(AActor* Actor, UDA_CoreCraftingRecipe* Recipe, FS_InventoryItem CreatedItem);

	/**Called by UDA_CoreCraftingRecipe -> LoadSoftReferences
	 * This lets you load any soft references this object might
	 * be referencing.*/
	UFUNCTION(Category = "Events", BlueprintCallable, BlueprintImplementableEvent)
	void LoadAssets();
};
