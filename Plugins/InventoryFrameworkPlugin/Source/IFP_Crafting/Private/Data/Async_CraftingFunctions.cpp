// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Data/Async_CraftingFunctions.h"
#include "Engine/AssetManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Recipes/DataAssets/DA_CoreCraftingRecipe.h"

UAsync_GetRecipesRequiringItem* UAsync_GetRecipesRequiringItem::GetRecipesRequiringItem(UObject* Context, TSoftObjectPtr<UDA_CoreItem> ItemAsset,
	FPrimaryAssetType AssetType, TArray<FName> AssetBundles)
{
	UAsync_GetRecipesRequiringItem* NewAsyncObject = NewObject<UAsync_GetRecipesRequiringItem>(Context);
	NewAsyncObject->Item = ItemAsset;
	NewAsyncObject->Asset = AssetType;
	NewAsyncObject->Bundles = AssetBundles;
	return NewAsyncObject;
}

void UAsync_GetRecipesRequiringItem::Activate()
{
	Super::Activate();

	// Get the Asset Manager from anywhere
	UAssetManager* Manager = UAssetManager::GetIfInitialized();
	if (!Manager)
	{
		RemoveFromRoot();
		return;
	}

	TArray<FPrimaryAssetId> AssetList;
	UKismetSystemLibrary::GetPrimaryAssetIdList(Asset, AssetList);
	if(AssetList.IsEmpty())
	{
		//No assets found, just end here
		RemoveFromRoot();
		return;
	}
	//Set loading required so we know when to remove this object.
	LoadingRequired = AssetList.Num();
	
	for(auto& CurrentAsset : AssetList)
	{
		FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(this, &UAsync_GetRecipesRequiringItem::OnRecipeLoaded, CurrentAsset);
		Manager->LoadPrimaryAsset(CurrentAsset, Bundles, Delegate);
	}
}

void UAsync_GetRecipesRequiringItem::OnRecipeLoaded(FPrimaryAssetId LoadedID)
{
	UAssetManager* Manager = UAssetManager::GetIfInitialized();
	if (!Manager)
	{
		RemoveFromRoot();
		return;
	}

	UDA_CoreCraftingRecipe* Recipe = Cast<UDA_CoreCraftingRecipe>(Manager->GetPrimaryAssetObject(LoadedID));

	if(!Recipe)
	{
		RemoveFromRoot();
		return;
	}

	int32 QuantityRequired = 0;
	for(auto& CurrentRequirement : Recipe->GetRequiredItems())
	{
		if(CurrentRequirement.Key == Item)
		{
			QuantityRequired = CurrentRequirement.Value;
			RecipeFound.Broadcast(Recipe, QuantityRequired);
			break;
		}
	}

	LoadedRecipes++;
	if(LoadedRecipes >= LoadingRequired)
	{
		RemoveFromRoot();
		Finished.Broadcast(Recipe, QuantityRequired);
	}
}

UAsync_GetRecipesForItem* UAsync_GetRecipesForItem::GetRecipesForItem(UObject* Context,
	TSoftObjectPtr<UDA_CoreItem> ItemAsset, FPrimaryAssetType AssetType, TArray<FName> AssetBundles)
{
	UAsync_GetRecipesForItem* NewAsyncObject = NewObject<UAsync_GetRecipesForItem>(Context);
	NewAsyncObject->Item = ItemAsset;
	NewAsyncObject->Asset = AssetType;
	return NewAsyncObject;
}

void UAsync_GetRecipesForItem::Activate()
{
	Super::Activate();

	// Get the Asset Manager from anywhere
	UAssetManager* Manager = UAssetManager::GetIfInitialized();
	if (!Manager)
	{
		RemoveFromRoot();
		return;
	}

	TArray<FPrimaryAssetId> AssetList;
	UKismetSystemLibrary::GetPrimaryAssetIdList(Asset, AssetList);
	if(AssetList.IsEmpty())
	{
		//No assets found, just end here
		RemoveFromRoot();
		return;
	}
	//Set loading required so we know when to remove this object.
	LoadingRequired = AssetList.Num();
	
	for(auto& CurrentAsset : AssetList)
	{
		FStreamableDelegate Delegate = FStreamableDelegate::CreateUObject(this, &UAsync_GetRecipesForItem::OnRecipeLoaded, CurrentAsset);
		Manager->LoadPrimaryAsset(CurrentAsset, Bundles, Delegate);
	}
}

void UAsync_GetRecipesForItem::OnRecipeLoaded(FPrimaryAssetId LoadedID)
{
	UAssetManager* Manager = UAssetManager::GetIfInitialized();
	if (!Manager)
	{
		RemoveFromRoot();
		return;
	}

	UDA_CoreCraftingRecipe* Recipe = Cast<UDA_CoreCraftingRecipe>(Manager->GetPrimaryAssetObject(LoadedID));

	if(!Recipe)
	{
		RemoveFromRoot();
		return;
	}

	if(Recipe->ItemToCraft == Item)
	{
		FoundRecipe.Broadcast(Recipe);
	}

	LoadedRecipes++;
	if(LoadedRecipes >= LoadingRequired)
	{
		RemoveFromRoot();
		Finish.Broadcast(Recipe);
	}
}
