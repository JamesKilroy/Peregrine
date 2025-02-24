// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Components/AC_Crafting.h"

#include "Core/Components/AC_Inventory.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Interfaces/I_Inventory.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Net/UnrealNetwork.h"
#include "Recipes/RecipeData/RD_TimeToCraft.h"


UAC_Crafting::UAC_Crafting()
{
	PrimaryComponentTick.bCanEverTick = 1;
	PrimaryComponentTick.bTickEvenWhenPaused = 1;
	PrimaryComponentTick.bStartWithTickEnabled = 1;
	bAutoActivate = true;
}

void UAC_Crafting::BeginPlay()
{
	Super::BeginPlay();
	/**Tick logic is irrelevant on clients. Disable it if we have
	 * no authority. */
	SetComponentTickEnabled(GetOwner()->HasAuthority());
}

void UAC_Crafting::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UAC_Crafting, Tags, COND_OwnerOnly)
	DOREPLIFETIME_CONDITION(UAC_Crafting, Recipes, COND_OwnerOnly)
}

void UAC_Crafting::OnRep_Recipes_Internal(const TArray<TSoftObjectPtr<UDA_CoreCraftingRecipe>>& OldRecipes)
{
	//Call blueprint event for blueprint programmers
	OnRep_Recipes(OldRecipes);
}

void UAC_Crafting::OnRep_Tags_Internal(FGameplayTagContainer OldTags)
{
	UpdateDisplayedRecipes.Broadcast();
	UpdateCraftableRecipes.Broadcast();

	//Call blueprint event for blueprint programmers
	OnRep_Tags(OldTags);
}

UAC_Crafting* UAC_Crafting::GetCraftingComponent(AActor* Actor)
{
	if(!IsValid(Actor))
	{
		return nullptr;
	}

	/**
	 * Insert any custom interface system here
	 */

	return Cast<UAC_Crafting>(Actor->GetComponentByClass(UAC_Crafting::StaticClass()));
}

void UAC_Crafting::TickComponent(float DeltaTime, enum ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for(int32 CurrentCraft = 0; CurrentCraft < CraftHandles.Num(); CurrentCraft++)
	{
		if(CraftHandles[CurrentCraft].StartTime + CraftHandles[CurrentCraft].TimeToCraft > UGameplayStatics::GetUnpausedTimeSeconds(this))
		{
			continue;
		}
		
		UDA_CoreItem* ItemToCraft = CraftHandles[CurrentCraft].Recipe->ItemToCraft.LoadSynchronous();
		if(!IsValid(ItemToCraft))
		{
			return;
		}
		
		/**We rerun some of the validation if this is a timed craft,
		 * since things might have changed since the call of this function
		 * and when it is about to finish. */
		if(!UKismetSystemLibrary::DoesImplementInterface(GetOwner(), UI_Inventory::StaticClass()))
		{
			C_CraftCancelled(CraftHandles[CurrentCraft].Recipe, CraftHandles[CurrentCraft].TimerHandle, true);
			return;
		}

		if(CraftHandles[CurrentCraft].Inventory == nullptr)
		{
			C_CraftCancelled(CraftHandles[CurrentCraft].Recipe, CraftHandles[CurrentCraft].TimerHandle, true);
			return;
		}
			
		if(!CraftHandles[CurrentCraft].Recipe->DoesActorMeetCraftRequirements(GetOwner()))
		{
			/**Client has most likely done something during the timer
			 * that caused them to be invalid. For example, requiring
			 * a certain tag to be able to craft this recipe, but that
			 * tag has been removed since they started this craft.*/
			C_CraftCancelled(CraftHandles[CurrentCraft].Recipe, CraftHandles[CurrentCraft].TimerHandle, true);
			return;
		}

		/**Notify all objects about the craft.
		 * This function does not handle any important code,
		 * this is for a few reasons:
		 * 1. Prevents this function from getting omega-spaghetti
		 * if you want a lot of dynamic code to happen.
		 * 2. Blueprint programmers gain very easy access to
		 * insert any blueprint code.
		 *
		 * If you want to stay inside C++, all the objects can
		 * still be made at a C++ level.*/
		TArray<UO_RecipeObject*> RecipeObjects = CraftHandles[CurrentCraft].Recipe->GetAllObjects();
		
		for(auto& CurrentObject : RecipeObjects)
		{
			if(!UKismetSystemLibrary::IsStandalone(this))
			{
				if(UO_CoreCraftEvent* CraftEvent = Cast<UO_CoreCraftEvent>(CurrentObject))
				{
					if(CraftEvent->NetworkingType == Client && UKismetSystemLibrary::IsDedicatedServer(this))
					{
						continue;
					}
				}
			}
		
			CurrentObject->PreRecipeCrafted(GetOwner(), CraftHandles[CurrentCraft].Recipe);
		}
		
		C_CallPreRecipeCrafted(CraftHandles[CurrentCraft].Recipe);

		//Start adding the item
		FS_InventoryItem NewItem;
		NewItem.ItemAsset = ItemToCraft;
		NewItem.Count = CraftHandles[CurrentCraft].Recipe->ItemCount;
		bool Success = false;
		int32 CountDelta = 0;

		TArray<FS_ContainerSettings> ItemsContainers = ItemToCraft->GetDefaultContainers();

		//If this fails to add the item, then there might be something wrong with the
		//requirements setup for your recipe.
		//In some cases, you might want to create a backup system, such as a "mail" system
		//where if a spot was failed to be found, but you still consumed all the items
		//required to craft the recipe, you can then mail the item to the player
		//or store it in a different, more safe place.
		CraftHandles[CurrentCraft].Inventory->TryAddNewItem(NewItem, ItemsContainers, CraftHandles[CurrentCraft].Inventory, true, false, Success, NewItem, CountDelta);

		RecipeCrafted.Broadcast(GetOwner(), CraftHandles[CurrentCraft].Recipe, NewItem, CraftHandles[CurrentCraft].TimerHandle);

		for(auto& CurrentObject : RecipeObjects)
		{
			
			if(UO_CoreCraftEvent* CraftEvent = Cast<UO_CoreCraftEvent>(CurrentObject))
			{
				if(CraftEvent->NetworkingType == Client)
				{
					continue;
				}
			}
			
			CurrentObject->PostRecipeCrafted(GetOwner(), CraftHandles[CurrentCraft].Recipe, NewItem);
		}
		
		C_CallPostRecipeCrafted(CraftHandles[CurrentCraft].Recipe, NewItem.UniqueID, CraftHandles[CurrentCraft].TimerHandle);

		CraftHandles.RemoveAt(CurrentCraft);
		CurrentCraft--;
	}
}

int32 UAC_Crafting::GetUniqueCraftHandle()
{
	CraftHandle++;
	if(CraftHandle == 2147483646)
	{
		//Loop back to 0 if we reach int32 limit
		CraftHandle = 0;
	}
	
	return CraftHandle;
}

bool UAC_Crafting::IsHandleUnique(int32 Handle)
{
	for(auto& CurrentHandle : CraftHandles)
	{
		if(CurrentHandle.TimerHandle == Handle)
		{
			return false;
		}
	}

	return true;
}

void UAC_Crafting::C_CraftCancelled_Implementation(UDA_CoreCraftingRecipe* Recipe, int32 Handle,
	bool FailedValidation)
{
	CraftCancelled.Broadcast(Recipe, Handle, FailedValidation);
}

void UAC_Crafting::S_CancelCraft_Implementation(int32 Handle)
{
	for(int32 CurrentCraft = 0; CurrentCraft < CraftHandles.Num(); CurrentCraft++)
	{
		if(CraftHandles[CurrentCraft].TimerHandle == Handle)
		{
			CraftHandles.RemoveAt(CurrentCraft);
			C_CraftCancelled(CraftHandles[CurrentCraft].Recipe, CraftHandles[CurrentCraft].TimerHandle, false);
			return;
		}
	}
}

void UAC_Crafting::S_CraftRecipe_Implementation(UDA_CoreCraftingRecipe* Recipe, int32 Handle)
{
	//Start of validation
	if(!UKismetSystemLibrary::DoesImplementInterface(GetOwner(), UI_Inventory::StaticClass()))
	{
		return;
	}

	UAC_Inventory* Inventory = nullptr;
	II_Inventory::Execute_GetInventoryComponent(GetOwner(), Inventory);

	if(Inventory == nullptr)
	{
		return;
	}
	
	if(!Recipe->DoesActorMeetCraftRequirements(GetOwner()))
	{
		/**Suspicious behavior or client might be lagging.
		 * Might want to consider checking for cheating here
		 * if they can call this server function.*/
		return;
	}

	UDA_CoreItem* ItemToCraft = Recipe->ItemToCraft.LoadSynchronous();
	if(!IsValid(ItemToCraft))
	{
		return;
	}
	//end of validation

	/**Notify all objects about the craft.
	 * This function does not handle any important code,
	 * this is for a few reasons:
	 * 1. Prevents this function from getting omega-spaghetti
	 * if you want a lot of dynamic code to happen.
	 * 2. Blueprint programmers gain very easy access to
	 * insert any blueprint code.
	 *
	 * If you want to stay inside C++, all the objects can
	 * still be made at a C++ level.*/
	TArray<UO_RecipeObject*> RecipeObjects = Recipe->GetAllObjects();

	/**Evaluate if this is a timed craft.
	 * If so, then store the timer handle and the @Handle
	 * so clients can request the cancellation of the craft.*/
	FTimerHandle TimerHandle;
	URD_TimeToCraftBase* TimeToCraftData = Cast<URD_TimeToCraftBase>(Recipe->GetRecipeDataByClass(URD_TimeToCraft::StaticClass()));;
	if(TimeToCraftData && Handle > -1)
	{
		if(IsHandleUnique(Handle))
		{
			FTimedCraft TimedCraft;
			TimedCraft.Inventory = Inventory;
			TimedCraft.Recipe = Recipe;
			TimedCraft.StartTime = UGameplayStatics::GetUnpausedTimeSeconds(this);
			TimedCraft.TimeToCraft = TimeToCraftData->GetTimeToCraft();
			TimedCraft.TimerHandle = Handle;
			CraftHandles.Add(TimedCraft);
			//Return here and allow the tick function to finish the craft
			return;
		}
		else
		{
			UFL_InventoryFramework::LogIFPMessage(this,
				"Tried to pass in handle that wasn't unique, cancelling craft. Please use the GetUniqueHandle function - S_CraftRecipe");
			return;
		}
	}
	
	for(auto& CurrentObject : RecipeObjects)
	{
		if(!UKismetSystemLibrary::IsStandalone(this))
		{
			if(UO_CoreCraftEvent* CraftEvent = Cast<UO_CoreCraftEvent>(CurrentObject))
			{
				if(CraftEvent->NetworkingType == Client && UKismetSystemLibrary::IsDedicatedServer(this))
				{
					continue;
				}
			}
		}
	
		CurrentObject->PreRecipeCrafted(GetOwner(), Recipe);
	}
	
	C_CallPreRecipeCrafted(Recipe);

	//Start adding the item
	FS_InventoryItem NewItem;
	NewItem.ItemAsset = ItemToCraft;
	NewItem.Count = Recipe->ItemCount;
	bool Success = false;
	int32 CountDelta = 0;

	TArray<FS_ContainerSettings> ItemsContainers = ItemToCraft->GetDefaultContainers();

	//If this fails to add the item, then there might be something wrong with the
	//requirements setup for your recipe.
	//In some cases, you might want to create a backup system, such as a "mail" system
	//where if a spot was failed to be found, but you still consumed all the items
	//required to craft the recipe, you can then mail the item to the player
	//or store it in a different, more safe place.
	Inventory->TryAddNewItem(NewItem, ItemsContainers, Inventory, true, false, Success, NewItem, CountDelta);

	RecipeCrafted.Broadcast(GetOwner(), Recipe, NewItem, Handle);

	for(auto& CurrentObject : RecipeObjects)
	{
		
		if(UO_CoreCraftEvent* CraftEvent = Cast<UO_CoreCraftEvent>(CurrentObject))
		{
			if(CraftEvent->NetworkingType == Client)
			{
				continue;
			}
		}
		
		CurrentObject->PostRecipeCrafted(GetOwner(), Recipe, NewItem);
	}
	
	C_CallPostRecipeCrafted(Recipe, NewItem.UniqueID, Handle);
}

void UAC_Crafting::C_CallPreRecipeCrafted_Implementation(UDA_CoreCraftingRecipe* Recipe)
{
	if(!UKismetSystemLibrary::IsStandalone(this) && !UKismetSystemLibrary::IsServer(this))
	{
		for(const auto& CurrentEvent : Recipe->CraftEvents)
		{
			if(CurrentEvent->NetworkingType != Server)
			{
				CurrentEvent->PreRecipeCrafted(GetOwner(), Recipe);
			}
		}
	}
}

void UAC_Crafting::C_CallPostRecipeCrafted_Implementation(UDA_CoreCraftingRecipe* Recipe, FS_UniqueID ItemCreatedUniqueID, int32 Handle)
{
	if(!IsValid(ItemCreatedUniqueID.ParentComponent))
	{
		return;
	}
	
	FS_InventoryItem Item = ItemCreatedUniqueID.ParentComponent->GetItemByUniqueID(ItemCreatedUniqueID);
	if(!Item.IsValid())
	{
		return;
	}
	
	for(const auto& CurrentEvent : Recipe->CraftEvents)
	{
		if(CurrentEvent->NetworkingType == Client)
		{
			CurrentEvent->PostRecipeCrafted(GetOwner(), Recipe, Item);
		}
	}
	
	if(!UKismetSystemLibrary::IsStandalone(this) && !UKismetSystemLibrary::IsServer(this))
	{
		RecipeCrafted.Broadcast(GetOwner(), Recipe, Item, Handle);
	}
}

void UAC_Crafting::S_AddRecipe_Implementation(UDA_CoreCraftingRecipe* Recipe)
{
	if(Recipes.Contains(Recipe))
	{
		//Recipe was already owned
		return;
	}
	
	Recipes.Add(Recipe);
	RecipeAdded.Broadcast(Recipe);

	
	if(!UKismetSystemLibrary::IsStandalone(this))
	{
		//ForceNetUpdate should ensure that the Recipe's
		//array is replicated by the time the RPC goes through
		GetOwner()->ForceNetUpdate();
		C_CallRecipeAdded(Recipe);
	}
}

void UAC_Crafting::C_CallRecipeAdded_Implementation(UDA_CoreCraftingRecipe* Recipe)
{
	if(!UKismetSystemLibrary::IsStandalone(this) && !UKismetSystemLibrary::IsServer(this))
	{
		RecipeAdded.Broadcast(Recipe);
	}
}


void UAC_Crafting::S_RemoveRecipe_Implementation(UDA_CoreCraftingRecipe* Recipe)
{
	if(!Recipes.RemoveSingle(Recipe))
	{
		//Recipe was not found for removal.
		return;
	}

	RecipeRemoved.Broadcast(Recipe);

	
	if(!UKismetSystemLibrary::IsStandalone(this))
	{
		//ForceNetUpdate should ensure that the Recipe's
		//array is replicated by the time the RPC goes through
		GetOwner()->ForceNetUpdate();
		C_CallRecipeRemoved(Recipe);
	}
}

void UAC_Crafting::C_CallRecipeRemoved_Implementation(UDA_CoreCraftingRecipe* Recipe)
{
	if(!UKismetSystemLibrary::IsStandalone(this) && !UKismetSystemLibrary::IsServer(this))
	{
		RecipeRemoved.Broadcast(Recipe);
	}
}

TArray<UDA_CoreCraftingRecipe*> UAC_Crafting::LoadAndGetRecipes()
{
	TArray<UDA_CoreCraftingRecipe*> LoadedRecipes;
	
	for(auto& CurrentRecipe : Recipes)
	{
		LoadedRecipes.Add(CurrentRecipe.LoadSynchronous());
	}

	return LoadedRecipes;
}