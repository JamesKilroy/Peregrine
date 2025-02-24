// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Async_CraftingFunctions.generated.h"

class UDA_CoreCraftingRecipe;
class UDA_CoreItem;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFinished, TSoftObjectPtr<UDA_CoreCraftingRecipe>, Recipe, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRecipeFound, TSoftObjectPtr<UDA_CoreCraftingRecipe>, Recipe, int32, Quantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFinish, TSoftObjectPtr<UDA_CoreCraftingRecipe>, Recipe);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFoundRecipe, TSoftObjectPtr<UDA_CoreCraftingRecipe>, Recipe);

/**
 * 
 */
UCLASS()
class IFP_CRAFTING_API UAsync_GetRecipesRequiringItem : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FFinished Finished;

	UPROPERTY(BlueprintAssignable)
	FRecipeFound RecipeFound;

	UPROPERTY()
	TSoftObjectPtr<UDA_CoreItem> Item;

	UPROPERTY()
	FPrimaryAssetType Asset;

	//Used to find out when loading is done so we can remove this object.
	UPROPERTY()
	int32 LoadedRecipes = 0;

	UPROPERTY()
	int32 LoadingRequired = 0;

	UPROPERTY()
	TArray<FName> Bundles;

	/**Return all recipe's found in the database associated with @AssetType that
	 * require @ItemAsset in their crafting requirements and how much is required.
	 * 
	 * @AssetBundles what assets do you want to load alongside the recipe?
	 * If you include "ItemToCraft", it'll also load the item asset.
	 * 
	 * This is asynchronous, so order is not consistent. Whatever your computer
	 * decides to load first will appear first.
	 *
	 * This does load EVERY recipe with the @AssetType, as that is required
	 * to check its required items. If this is causing a hitch, it might
	 * be more worthwhile to add a trait to the @ItemAsset which stores
	 * a soft reference to the recipe's, though that will require manual
	 * work, where as this is automatic.*/
	UFUNCTION(Category="IFP|Crafting", BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="Context"))
	static UAsync_GetRecipesRequiringItem* GetRecipesRequiringItem(UObject* Context, TSoftObjectPtr<UDA_CoreItem> ItemAsset, FPrimaryAssetType AssetType, TArray<FName> AssetBundles);

	virtual void Activate() override;

	void OnRecipeLoaded(FPrimaryAssetId LoadedID);
};

UCLASS()
class IFP_CRAFTING_API UAsync_GetRecipesForItem : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintAssignable)
	FFinish Finish;

	UPROPERTY(BlueprintAssignable)
	FFoundRecipe FoundRecipe;

	UPROPERTY()
	TSoftObjectPtr<UDA_CoreItem> Item;

	UPROPERTY()
	FPrimaryAssetType Asset;

	//Used to find out when loading is done so we can remove this object.
	UPROPERTY()
	int32 LoadedRecipes = 0;

	UPROPERTY()
	int32 LoadingRequired = 0;

	UPROPERTY()
	TArray<FName> Bundles;

	/**Return all recipe's found in the database associated with @AssetType that
	 * will craft @ItemAsset.
	 * 
	 * @AssetBundles what assets do you want to load alongside the recipe?
	 * If you include "ItemToCraft", it'll also load the item asset.
	 * 
	 * This is asynchronous, so order is not consistent. Whatever your computer
	 * decides to load first will appear first.
	 *
	 *
	 * This does load EVERY recipe with the @AssetType, as that is required
	 * to check its required items. If this is causing a hitch, it might
	 * be more worthwhile to add a trait to the @ItemAsset which stores
	 * a soft reference to the recipe's, though that will require manual
	 * work, where as this is automatic.*/
	UFUNCTION(Category="IFP|Crafting", BlueprintCallable, meta=(BlueprintInternalUseOnly="true", WorldContext="Context"))
	static UAsync_GetRecipesForItem* GetRecipesForItem(UObject* Context, TSoftObjectPtr<UDA_CoreItem> ItemAsset, FPrimaryAssetType AssetType, TArray<FName> AssetBundles);

	virtual void Activate() override;

	void OnRecipeLoaded(FPrimaryAssetId LoadedID);
};