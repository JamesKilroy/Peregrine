// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Items/DA_CoreItem.h"
#include "Engine/DataAsset.h"
#include "Recipes/CraftEvents/O_CoreCraftEvent.h"
#include "Recipes/RecipeData/O_CoreRecipeData.h"
#include "Recipes/RecipeDisplay/O_CoreRecipeDisplay.h"
#include "Recipes/RecipeRequirements/O_CoreRecipeRequirement.h"
#include "DA_CoreCraftingRecipe.generated.h"

/**The base crafting recipe asset. It is possible to create
 * children of this asset to create more specialized recipes.
 *
 * Docs: https://inventoryframework.github.io/crafting/*/
UCLASS(Blueprintable, BlueprintType)
class IFP_CRAFTING_API UDA_CoreCraftingRecipe : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	//--------------------
	// Variables

	UPROPERTY(Category = "Recipe", BlueprintReadOnly, EditAnywhere, AssetRegistrySearchable,  meta = (AssetBundles = "ItemToCraft"))
	TSoftObjectPtr<UDA_CoreItem> ItemToCraft = nullptr;

	/**How many of @ItemToCraft should be granted to the player?*/
	UPROPERTY(Category = "Recipe", BlueprintReadOnly, EditAnywhere)
	int32 ItemCount = 1;

	/**Additional requirements needed to craft this item, such as
	 * a player level or a crafting skill.*/
	UPROPERTY(Category = "Recipe", BlueprintReadOnly, EditAnywhere, Instanced)
	TArray<UO_CoreRecipeRequirement*> RecipeRequirements;

	/**Code to execute when the item is successfully crafted,z
	 * such as a sound effect or increasing a crafting skill.*/
	UPROPERTY(Category = "Recipe", BlueprintReadOnly, EditAnywhere, Instanced)
	TArray<UO_CoreCraftEvent*> CraftEvents;

	/**Optional data to associate with this asset.*/
	UPROPERTY(Category = "Recipe", BlueprintReadOnly, EditAnywhere, Instanced)
	TArray<UO_CoreRecipeData*> RecipeData;
	
	/**Requirements needed to be met for this recipe to be displayed
	 * in the crafting menu.*/
	UPROPERTY(Category = "Recipe", BlueprintReadOnly, EditAnywhere, Instanced)
	TArray<UO_CoreRecipeDisplay*> DisplayRequirements;

	//--------------------

	
	//--------------------
	// Functions

	UFUNCTION(Category = "Crafting|Requirements", BlueprintCallable)
	bool DoesActorMeetCraftRequirements(AActor* Actor);

	UFUNCTION(Category = "Crafting|Requirements", BlueprintCallable)
	bool DoesActorMeetDisplayRequirements(AActor* Actor);

	UFUNCTION(Category = "Crafting|Requirements", BlueprintCallable, BlueprintPure, meta=(DeterminesOutputType="Class"))
	UO_CoreRecipeRequirement* GetCraftRequirementByClass(TSubclassOf<UO_CoreRecipeRequirement> Class);

	UFUNCTION(Category = "Crafting|Requirements", BlueprintCallable, BlueprintPure, meta=(DeterminesOutputType="Class"))
	UO_CoreRecipeDisplay* GetDisplayRequirementByClass(TSubclassOf<UO_CoreRecipeDisplay> Class);

	UFUNCTION(Category = "Crafting|Requirements", BlueprintCallable, BlueprintPure, meta=(DeterminesOutputType="Class"))
	UO_CoreCraftEvent* GetCraftEventByClass(TSubclassOf<UO_CoreCraftEvent> Class);

	UFUNCTION(Category = "Crafting|Requirements", BlueprintCallable, BlueprintPure, meta=(DeterminesOutputType="Class"))
	UO_CoreRecipeData* GetRecipeDataByClass(TSubclassOf<UO_CoreRecipeData> Class);
	
	UFUNCTION(Category = "Crafting|Requirements", BlueprintCallable, BlueprintPure)
	TArray<UO_RecipeObject*> GetAllObjects();

	/**Get all items this recipe requires and the quantity required.*/
	UFUNCTION(Category = "Crafting|Requirements", BlueprintCallable, BlueprintPure)
	TMap<TSoftObjectPtr<UDA_CoreItem>, int32> GetRequiredItems();

	/**Go through all objects and call LoadAssets*/
	UFUNCTION(Category = "Crafting", BlueprintCallable)
	void LoadSoftReferences();

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	//--------------------
};
