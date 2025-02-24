// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Recipes/DataAssets/DA_CoreCraftingRecipe.h"
#include "AC_Crafting.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRecipeAdded, TSoftObjectPtr<UDA_CoreCraftingRecipe>, Recipe);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FRecipeRemoved, TSoftObjectPtr<UDA_CoreCraftingRecipe>, Recipe);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FRecipeCrafted, AActor*, Actor, TSoftObjectPtr<UDA_CoreCraftingRecipe>, Recipe, FS_InventoryItem, ItemCreated, int32, Handle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FCraftCancelled, UDA_CoreCraftingRecipe*, Recipe, int32, Handle, bool, FailedValidation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUpdateDisplayedRecipes);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FUpdateCraftableRecipes);

USTRUCT(BlueprintType)
struct FTimedCraft
{
	GENERATED_BODY()

	UPROPERTY(Category = "Timed Craft", EditAnywhere)
	int32 TimerHandle = -1;

	UPROPERTY(Category = "Timed Craft", EditAnywhere)
	float StartTime = 0;

	UPROPERTY(Category = "Timed Craft", EditAnywhere)
	float TimeToCraft = 0;
	
	UPROPERTY(Category = "Timed Craft", EditAnywhere)
	UDA_CoreCraftingRecipe* Recipe = nullptr;

	UPROPERTY(Category = "Timed Craft", EditAnywhere)
	TObjectPtr<UAC_Inventory> Inventory = nullptr;

	FTimedCraft() {}
	
	FTimedCraft(int32 InTimerHandle, UDA_CoreCraftingRecipe* InRecipe)
	{
		TimerHandle = InTimerHandle;
		Recipe = InRecipe;
	}
};

UCLASS(Blueprintable, ClassGroup=(IFP), meta=(BlueprintSpawnableComponent), DisplayName = "Crafting Component")
class IFP_CRAFTING_API UAC_Crafting : public UActorComponent
{
	GENERATED_BODY()

	int32 CraftHandle = 0;

public:
	// Sets default values for this component's properties
	UAC_Crafting();
	
	virtual bool IsSupportedForNetworking () const override { return true; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//--------------------
	// Variables

	/**What recipe's does the owner of this component have unlocked?*/
	UPROPERTY(Category = "Recipe", BlueprintReadWrite, EditAnywhere, ReplicatedUsing = "OnRep_Recipes_Internal")
	TArray<TSoftObjectPtr<UDA_CoreCraftingRecipe>> Recipes;
	UFUNCTION()
	void OnRep_Recipes_Internal(const TArray<TSoftObjectPtr<UDA_CoreCraftingRecipe>>& OldRecipes);
	UFUNCTION(BlueprintImplementableEvent)
	void OnRep_Recipes(const TArray<TSoftObjectPtr<UDA_CoreCraftingRecipe>>& OldRecipes);

	/**Example tags used by recipe's to evaluate certain requirements.
	 * Ideally, you would have your own tag system.*/
	UPROPERTY(Category = "Tags", BlueprintReadWrite, EditAnywhere, ReplicatedUsing = "OnRep_Tags_Internal")
	FGameplayTagContainer Tags;
	UFUNCTION()
	void OnRep_Tags_Internal(FGameplayTagContainer OldTags);
	UFUNCTION(BlueprintImplementableEvent)
	void OnRep_Tags(FGameplayTagContainer OldTags);

	/**Craft timer handles that are currently in the process of being crafted.
	 * This is only populated on the server. */
	UPROPERTY(Category = "Crafting", BlueprintReadOnly)
	TArray<FTimedCraft> CraftHandles;

	//--------------------


	//--------------------
	// Delegates

	UPROPERTY(Category = "EventDispatchers", BlueprintAssignable, BlueprintCallable)
	FRecipeAdded RecipeAdded;

	UPROPERTY(Category = "EventDispatchers", BlueprintAssignable, BlueprintCallable)
	FRecipeRemoved RecipeRemoved;

	UPROPERTY(Category = "EventDispatchers", BlueprintAssignable, BlueprintCallable)
	FRecipeCrafted RecipeCrafted;

	UPROPERTY(Category = "EventDispatchers", BlueprintAssignable, BlueprintCallable)
	FCraftCancelled CraftCancelled;

	/**Called from various places to communicate with any widgets
	 * that would want to show or hide when a possible display
	 * requirement can or can not be met.*/
	UPROPERTY(Category = "EventDispatchers", BlueprintAssignable, BlueprintCallable)
	FUpdateDisplayedRecipes UpdateDisplayedRecipes;
	
	/**Called from various places to communicate with any widgets
	 * that would want to enable or disable when a possible craft
	 * requirement can or can not be met.*/
	UPROPERTY(Category = "EventDispatchers", BlueprintAssignable, BlueprintCallable)
	FUpdateCraftableRecipes UpdateCraftableRecipes;

	//--------------------


	//--------------------
	// Functions

	virtual void BeginPlay() override;

	/**Ideally a interface would be used, but to make it easier for users
	 * to remove the crafting system, this function is used instead.
	 * If you decide to use the crafting system, it is advised to create
	 * an interface, then have your player implement that interface and
	 * modify this function to use the interface.*/
	UFUNCTION(Category = "IFP|Crafting", BlueprintCallable, BlueprintPure, meta = (CompactNodeTitle = "Craft Component", DefaultToSelf = "Actor"))
	static UAC_Crafting* GetCraftingComponent(AActor* Actor);

	/**Attempt to craft the @Recipe, this will also check if the owner
	 * can or can not craft the item and notify all objects whether or not
	 * the craft was successful.
	 *
	 * Then finally, add the @Recipe -> @ItemToCraft into the owning actors
	 * inventory component and call RecipeCrafted for both server and client.
	 *
	 * @Handle This is used for timed crafts and allows client to cancel
	 * this craft. Call @GetUniqueCraftHandle, which will give you a unique
	 * handle to pass into this function, and then save it on the client,
	 * then you can cancel that craft under certain conditions, such
	 * as the player moving. */
	UFUNCTION(Category = "IFP|Crafting", BlueprintCallable, Server, Reliable)
	void S_CraftRecipe(UDA_CoreCraftingRecipe* Recipe, int32 Handle = -1);

	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/**Get a unique handle to pass into S_CraftRecipe. You will want to save this value.
	 * on the client so they can cancel the craft. */
	UFUNCTION(Category = "IFP|Crafting", BlueprintCallable, meta = (ReturnDisplayName = "Handle"))
	int32 GetUniqueCraftHandle();

	UFUNCTION(Category = "IFP|Crafting", BlueprintCallable, BlueprintPure)
	bool IsHandleUnique(int32 Handle);

	UFUNCTION(Category = "IFP|Crafting", BlueprintCallable, Server, Unreliable)
	void S_CancelCraft(int32 Handle);

	/**Notify the client that the cancellation of a craft
	 * has been acknowledged. Should be used to stop things
	 * like a progress bar.
	 *
	 * @FailedValidation returns true if the server evaluated
	 * that the client was no longer viable for the craft.
	 * For example, requiring a crafting station, then
	 * moving away from the crafting station before the
	 * craft finished. This can be followed with an error
	 * sound effect. Returns false if the client requested
	 * the cancellation, like pressing the escape button. */
	UFUNCTION(Category = "IFP|Crafting", BlueprintCallable, Client, Reliable)
	void C_CraftCancelled(UDA_CoreCraftingRecipe* Recipe, int32 Handle, bool FailedValidation);

	/**Called by S_CraftRecipe if in multiplayer.*/
	UFUNCTION(Category = "IFP|Crafting", BlueprintCallable, Client, Reliable)
	void C_CallPreRecipeCrafted(UDA_CoreCraftingRecipe* Recipe);

	/**Called by S_CraftRecipe if in multiplayer.*/
	UFUNCTION(Category = "IFP|Crafting", BlueprintCallable, Client, Reliable)
	void C_CallPostRecipeCrafted(UDA_CoreCraftingRecipe* Recipe, FS_UniqueID ItemCreatedUniqueID, int32 Handle = -1);

	/**Add a recipe to this component.*/
	UFUNCTION(Category = "IFP|Crafting", BlueprintCallable, Server, Unreliable)
	void S_AddRecipe(UDA_CoreCraftingRecipe* Recipe);

	/**Called by S_AddRecipe if in multiplayer.*/
	UFUNCTION(Category = "IFP|Crafting", BlueprintCallable, Client, Reliable)
	void C_CallRecipeAdded(UDA_CoreCraftingRecipe* Recipe);

	UFUNCTION(Category = "IFP|Crafting", BlueprintCallable, Server, Unreliable)
	void S_RemoveRecipe(UDA_CoreCraftingRecipe* Recipe);

	/**Called by S_RemoveRecipe if in multiplayer.*/
	UFUNCTION(Category = "IFP|Crafting", BlueprintCallable, Client, Reliable)
	void C_CallRecipeRemoved(UDA_CoreCraftingRecipe* Recipe);

	/**Instantly load and retrieve all recipes.
	 * This might be expensive depending on how many recipe's
	 * this component has. Generally advised to not use this
	 * unless your game is small to medium sized and isn't
	 * hard referencing any large assets.*/
	UFUNCTION(Category = "IFP|Crafting", BlueprintCallable, BlueprintPure, meta = (CompactNodeTitle = "Recipe's"))
	TArray<UDA_CoreCraftingRecipe*> LoadAndGetRecipes();
	
	//--------------------
};
