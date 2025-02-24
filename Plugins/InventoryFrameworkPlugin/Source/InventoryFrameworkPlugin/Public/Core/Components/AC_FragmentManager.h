// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Components/ActorComponent.h"
#include "Core/Data/IFP_CoreData.h"
#include "AC_FragmentManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFragmentAddedToItem, FS_InventoryItem, Item, FCoreFragment, Fragment);

UCLASS(ClassGroup=(IFP), meta=(BlueprintSpawnableComponent))
class INVENTORYFRAMEWORKPLUGIN_API UAC_FragmentManager : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UAC_FragmentManager();

	UPROPERTY()
	TObjectPtr<UAC_Inventory> Inventory = nullptr;
	UFUNCTION(Category = "Fragment Manager", BlueprintPure)
	UAC_Inventory* GetInventory();

	UFUNCTION(Category = "Fragment Manager", BlueprintNativeEvent)
	void InitializeFragments();

	virtual bool ValidateInstancedStructAsFragment(TInstancedStruct<FCoreFragment> Fragment);

	virtual bool ValidateFragmentsReplicationConditions(EObjectNetworkingMethod NetworkingMethod, TInstancedStruct<FCoreFragment> Fragment);

	/**Attempt to add a fragment to an item. This can fail if the item
	 *already has the fragment or if it's incompatible with the item.*/
	UFUNCTION(Category = "Fragment Manager|Items", BlueprintCallable)
	virtual void AddFragmentToItem(FS_InventoryItem Item, TInstancedStruct<FCoreFragment> Fragment);

	UFUNCTION(Server, WithValidation, Reliable)
	void S_AddFragmentToItem(FS_UniqueID ItemID, TInstancedStruct<FCoreFragment> Fragment, ENetRole CallerLocalRole);

	UFUNCTION(Client, Reliable)
	void C_AddFragmentToItem(FS_UniqueID ItemID, TInstancedStruct<FCoreFragment> Fragment);
	
	void Internal_AddFragmentToItem(FS_UniqueID ItemID, TInstancedStruct<FCoreFragment> Fragment);

	/**Attempt to remove a fragment from an item.*/
	UFUNCTION(Category = "Fragment Manager|Items", BlueprintCallable)
	virtual void RemoveFragmentFromItem(FS_InventoryItem Item, UScriptStruct* FragmentType);

	UFUNCTION(Server, WithValidation, Reliable)
	void S_RemoveFragmentFromItem(FS_UniqueID ItemID, UScriptStruct* FragmentType, ENetRole CallerLocalRole);

	UFUNCTION(Client, Reliable)
	void C_RemoveFragmentFromItem(FS_UniqueID ItemID, UScriptStruct* FragmentType);
	
	void Internal_RemoveFragmentFromItem(FS_UniqueID ItemID, UScriptStruct* FragmentType);
	
	/**Attempt to override a fragment on an item.*/
	UFUNCTION(Category = "Fragment Manager|Items", BlueprintCallable)
	virtual void OverrideFragmentOnItem(FS_InventoryItem Item, TInstancedStruct<FCoreFragment> Fragment, bool AddIfMissing = false);

	UFUNCTION(Server, WithValidation, Reliable)
	void S_OverrideFragmentOnItem(FS_UniqueID ItemID, TInstancedStruct<FCoreFragment> Fragment, ENetRole CallerLocalRole, bool AddIfMissing);

	UFUNCTION(Client, Reliable)
	void C_OverrideFragmentOnItem(FS_UniqueID ItemID, TInstancedStruct<FCoreFragment> Fragment, bool AddIfMissing);
	
	void Internal_OverrideFragmentOnItem(FS_UniqueID ItemID, TInstancedStruct<FCoreFragment> Fragment, bool AddIfMissing);

	/**Attempt to add a fragment to a container. This can fail if the item
	 *already has the fragment or if it's incompatible with the item.*/
	UFUNCTION(Category = "Fragment Manager|Containers", BlueprintCallable)
	virtual void AddFragmentToContainer(FS_ContainerSettings Container, TInstancedStruct<FCoreFragment> Fragment);

	UFUNCTION(Server, WithValidation, Reliable)
	void S_AddFragmentToContainer(FS_UniqueID ContainerID, TInstancedStruct<FCoreFragment> Fragment, ENetRole CallerLocalRole);

	UFUNCTION(Client, Reliable)
	void C_AddFragmentToContainer(FS_UniqueID ContainerID, TInstancedStruct<FCoreFragment> Fragment);
	
	void Internal_AddFragmentToContainer(FS_UniqueID ContainerID, TInstancedStruct<FCoreFragment> Fragment);

	/**Attempt to remove a fragment from a container.*/
	UFUNCTION(Category = "Fragment Manager|Containers", BlueprintCallable)
	virtual void RemoveFragmentFromContainer(FS_ContainerSettings Container, UScriptStruct* FragmentType);

	UFUNCTION(Server, WithValidation, Reliable)
	void S_RemoveFragmentFromContainer(FS_UniqueID ContainerID, UScriptStruct* FragmentType, ENetRole CallerLocalRole);

	UFUNCTION(Client, Reliable)
	void C_RemoveFragmentFromContainer(FS_UniqueID ContainerID, UScriptStruct* FragmentType);
	
	void Internal_RemoveFragmentFromContainer(FS_UniqueID ContainerID, UScriptStruct* FragmentType);
	
	/**Attempt to override a fragment on a container.*/
	UFUNCTION(Category = "Fragment Manager|Containers", BlueprintCallable)
	virtual void OverrideFragmentOnContainer(FS_ContainerSettings Container, TInstancedStruct<FCoreFragment> Fragment, bool AddIfMissing = false);

	UFUNCTION(Server, WithValidation, Reliable)
	void S_OverrideFragmentOnContainer(FS_UniqueID ContainerID, TInstancedStruct<FCoreFragment> Fragment, ENetRole CallerLocalRole, bool AddIfMissing);

	UFUNCTION(Client, Reliable)
	void C_OverrideFragmentOnContainer(FS_UniqueID ContainerID, TInstancedStruct<FCoreFragment> Fragment, bool AddIfMissing);
	
	void Internal_OverrideFragmentOnContainer(FS_UniqueID ContainerID, TInstancedStruct<FCoreFragment> Fragment, bool AddIfMissing);

#pragma region Getters

	UFUNCTION(Category = "Fragment Manager|Items", BlueprintCallable)
	TArray<FS_InventoryItem> GetItemsWithFragment(UScriptStruct* FragmentType);

	UFUNCTION(Category = "Fragment Manager|Containers", BlueprintCallable)
	TArray<FS_ContainerSettings> GetContainersWithFragment(UScriptStruct* FragmentType);
	
#pragma endregion
	
#if WITH_EDITOR

	/**How should the fragment manager react to any property changes?
	 * This is not made as a static function to allow for better overriding in children components.*/
	virtual void InventoryPostPropertyChange(FPropertyChangedEvent& PropertyChangedEvent, UAC_Inventory* InventoryComponent);
	
#endif
};
