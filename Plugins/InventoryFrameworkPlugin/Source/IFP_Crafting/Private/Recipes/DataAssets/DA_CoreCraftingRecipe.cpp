// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Recipes/DataAssets/DA_CoreCraftingRecipe.h"

#include "Core/Data/FL_InventoryFramework.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetSystemLibrary.h"

bool UDA_CoreCraftingRecipe::DoesActorMeetCraftRequirements(AActor* Actor)
{
	if(!IsValid(Actor))
	{
		UFL_InventoryFramework::LogIFPMessage(this, "No actor passed into DoesActorMeetCraftRequirements");
		return false;
	}
	
	//If there are no requirements, just instantly return true
	if(!RecipeRequirements.IsValidIndex(0))
	{
		return true;
	}

	for(auto& CurrentRequirement : RecipeRequirements)
	{
		if(!CurrentRequirement->CheckRequirements(Actor, this))
		{
			return false;
		}
	}

	return true;
}

bool UDA_CoreCraftingRecipe::DoesActorMeetDisplayRequirements(AActor* Actor)
{
	if(!IsValid(Actor))
	{
		UFL_InventoryFramework::LogIFPMessage(this, "No actor passed into DoesActorMeetDisplayRequirements");
		return false;
	}
	
	//If there are no requirements, just instantly return true
	if(!DisplayRequirements.IsValidIndex(0))
	{
		return true;
	}

	for(auto& CurrentDisplay : DisplayRequirements)
	{
		if(!CurrentDisplay->CheckRequirements(Actor, this))
		{
			return false;
		}
	}

	return true;
}

UO_CoreRecipeRequirement* UDA_CoreCraftingRecipe::GetCraftRequirementByClass(TSubclassOf<UO_CoreRecipeRequirement> Class)
{
	if(!Class || !RecipeRequirements.IsValidIndex(0))
	{
		return nullptr;
	}

	for(auto& CurrentRequirement : RecipeRequirements)
	{
		if(CurrentRequirement->GetClass() == Class)
		{
			return CurrentRequirement;
		}
	}

	return nullptr;
}

UO_CoreRecipeDisplay* UDA_CoreCraftingRecipe::GetDisplayRequirementByClass(TSubclassOf<UO_CoreRecipeDisplay> Class)
{
	if(!Class || !RecipeRequirements.IsValidIndex(0))
	{
		return nullptr;
	}

	for(auto& CurrentRequirement : DisplayRequirements)
	{
		if(CurrentRequirement->GetClass() == Class)
		{
			return CurrentRequirement;
		}
	}

	return nullptr;
}

UO_CoreCraftEvent* UDA_CoreCraftingRecipe::GetCraftEventByClass(TSubclassOf<UO_CoreCraftEvent> Class)
{
	if(!Class || !RecipeRequirements.IsValidIndex(0))
	{
		return nullptr;
	}

	for(auto& CurrentEvent : CraftEvents)
	{
		if(CurrentEvent->GetClass() == Class)
		{
			return CurrentEvent;
		}
	}

	return nullptr;
}

UO_CoreRecipeData* UDA_CoreCraftingRecipe::GetRecipeDataByClass(TSubclassOf<UO_CoreRecipeData> Class)
{
	if(!Class || !RecipeRequirements.IsValidIndex(0))
	{
		return nullptr;
	}

	for(auto& CurrentData : RecipeData)
	{
		if(CurrentData->GetClass() == Class)
		{
			return CurrentData;
		}
	}

	return nullptr;
}

TArray<UO_RecipeObject*> UDA_CoreCraftingRecipe::GetAllObjects()
{
	TArray<UO_RecipeObject*> FoundObjects;

	FoundObjects.Append(RecipeRequirements);
	FoundObjects.Append(CraftEvents);
	FoundObjects.Append(RecipeData);
	FoundObjects.Append(DisplayRequirements);

	return FoundObjects;
}

TMap<TSoftObjectPtr<UDA_CoreItem>, int32> UDA_CoreCraftingRecipe::GetRequiredItems()
{
	TMap<TSoftObjectPtr<UDA_CoreItem>, int32> FoundItems;
	for(const auto& CurrentRequirement : RecipeRequirements)
	{
		FoundItems.Append(CurrentRequirement->GetItemIngredients());
	}
	return FoundItems;
}

void UDA_CoreCraftingRecipe::LoadSoftReferences()
{
	TArray<UO_RecipeObject*> RecipeObjects = GetAllObjects();
	for(auto& CurrentObject : RecipeObjects)
	{
		CurrentObject->LoadAssets();
	}
}

FPrimaryAssetId UDA_CoreCraftingRecipe::GetPrimaryAssetId() const
{
	return FPrimaryAssetId("CraftingRecipes", GetFName());
}
