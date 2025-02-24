// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Interfaces/I_ExternalObjects.h"
#include "ItemComponent.generated.h"

class UIT_ItemComponentTrait;

/**An optional payload to pass into GetItemComponent and
 * various other functions.
 * These properties are completely unused by the plugin.
 * This means that this struct is 100% safe to add,
 * remove or modify for each projects' needs.*/
USTRUCT(BlueprintType)
struct FItemComponentPayload
{
	GENERATED_BODY()

	UPROPERTY(Category = "Payload", BlueprintReadWrite)
	FGameplayTagContainer Tags;

	UPROPERTY(Category = "Payload", BlueprintReadWrite)
	TArray<FS_TagValue> TagValues;

	UPROPERTY(Category = "Payload", BlueprintReadWrite)
	TObjectPtr<UObject> OptionalObject = nullptr;
};

//--------------------
// Event dispatchers
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnOwnerChanged, AActor*, OldOwner, AActor*, NewOwner);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEquipped, UItemComponent*, ItemComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUnequipped, UItemComponent*, ItemComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUsed, UItemComponent*, ItemComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAssetsLoaded, UItemComponent*, ItemComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FObjectConstructed, UItemComponent*, ItemComponent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FObjectDestroyed, UItemComponent*, ItemComponent);
//--------------------


/**An Actor component that is attached to an owner of an item.
 * This will then bind itself to the item and be accessible by the item,
 * allowing you to run gameplay logic, without creating an item actor.
 *
 * Docs:
 * inventoryframework.github.io/classes-and-settings/traitsandcomponents/*/

UCLASS(Abstract, Blueprintable,ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class INVENTORYFRAMEWORKPLUGIN_API UItemComponent : public UActorComponent, public II_ExternalObjects
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UItemComponent();

	
	//--------------------
	// Variables

	UPROPERTY(ReplicatedUsing = "OnRep_UniqueID", BlueprintReadWrite, meta=(ExposeOnSpawn=true), Category="UniqueID")
	FS_UniqueID UniqueID;
	UFUNCTION()
	void OnRep_UniqueID(FS_UniqueID OldUniqueID);

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Object")
	UIT_ItemComponentTrait* DataObject = nullptr;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Object")
	bool bIsBusy = false;

	UPROPERTY(Replicated, BlueprintReadWrite, Category = "Object")
	TObjectPtr<AActor> Instigator = nullptr;

	UPROPERTY(ReplicatedUsing = "OnRep_Owner", BlueprintReadWrite, meta=(ExposeOnSpawn=true), Category="UniqueID")
	TObjectPtr<AActor> Owner = nullptr;
	UFUNCTION()
	void OnRep_Owner(AActor* OldOwner);

	/**When was this ItemComponent created?
	 * This is used for item abilities to determine
	 * whether the ItemAbility instance is old or new.*/
	float CreationTime = 0;
	
	
	//--------------------
	// Networking
	
	//Label for network support.
	virtual bool IsSupportedForNetworking () const override { return true; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	
	//--------------------
	// Networking | Ownership
	
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Networking")
	void AssignNewOwner(AActor* NewOwner);

	/**Used to go from the server to the client to notify of any ownership change.*/
	UFUNCTION(Client, Reliable, Category = "Networking")
	void C_NotifyClientOfNewOwner(AActor* OldOwner, AActor* NewOwner);

	/**Called before new owner is assigned. The new owner has NOT been assigned, this is not called for clients.*/
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category = "Networking")
	void PreNewOwnerAssigned(AActor* OldOwner, AActor* NewOwner);

	/**A new owner has been assigned for the server.*/
	UFUNCTION(BlueprintImplementableEvent, BlueprintAuthorityOnly, Category = "Networking")
	void S_NewOwnerAssigned(AActor* OldOwner, AActor* NewOwner);

	/**A new owner has been assigned for the client.*/
	UFUNCTION(BlueprintImplementableEvent, Category = "Networking")
	void C_NewOwnerAssigned(AActor* OldOwner, AActor* NewOwner);
	
	
	//--------------------
	// Event Dispatchers

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FOnOwnerChanged OwnerChanged;
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FEquipped ItemEquipped;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FUnequipped ItemUnequipped;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FUsed OwningItemUsed;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FAssetsLoaded AssetsLoaded;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FObjectConstructed ObjectConstructed;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FObjectDestroyed ObjectDestroyed;

	
	//--------------------
	// Functions
	
	/*Gets the data of the item this object belongs to. You might want to save this in your functions as this
	 * has to go through all the items in the parent component until it finds one with a matching UniqueID,
	 * so frequent calls can take up unnecessary performance.
	 * WARNING: DO NOT use this in Multicast RPC's (Fine for client and server RPC's), other clients do not have the item data to get the data from this
	 * function. You also shouldn't send this data over the network, it has a lot of irrelevant data and is not optimized
	 * for network traffic.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters" , meta = (CompactNodeTitle = "ItemData"))
	FS_InventoryItem GetItemData();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters" , meta = (CompactNodeTitle = "Inventory"))
	UAC_Inventory* GetInventoryComponent();

	/**This is designed to be overwritten at a Blueprint level per-child or have defaults
	 * assigned in the parent.
	 * Allows you to call certain functions based on a tag. This is very useful for components
	 * that are set to ReplicationPolicy = Both, as you can now create a component for a client,
	 * pass in a tag and the component will immediately call the associated function the moment
	 * it gets replicated, for example; A healing item that plays an animation might
	 * get activated by a client by right clicking the item and pressing "Use", since
	 * the client is sending an RPC event to the server, the widget won't be able
	 * to get a reference to the component and the programmer will have to create some way
	 * of retrieving it. But if they pass in a tag that is associated with ItemUsed, then
	 * it'll get called once the server is done replicating.
	 *
	 * @Payload Optional payload to pass data from where ever this component was created from.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NativeFunctions")
	void ActivateEvent(FGameplayTag Event, FItemComponentPayload Payload);

	/**As a client, try to send ActivateEvent to the server*/
	UFUNCTION(BlueprintCallable, Server, Unreliable, Category = "Networking")
	void S_ActivateEvent(FGameplayTag Event, FItemComponentPayload Payload);

	/**Usually returns  bIsBusy, but this is here in case you want to have dynamic control
	 * to determine if this component is busy or not.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintPure, BlueprintCallable, Category = "NativeFunctions")
	bool IsBusy();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Event")
	void Equipped(const TArray<FName>& TriggerFilters);

	/**@OldItemData When moving an item, the equipment system has to know about
	 * the unequip event AFTER the item has been moved, but that means the old
	 * data has been deleted. This is an old copy, which should just be used to
	 * find the old components/actor the item had made so it can be removed.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Event")
	void Unequipped(const TArray<FName>& TriggerFilters, FS_InventoryItem OldItemData);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Event")
	void ItemUsed();

	/**Time to stop all logic and wipe all references. You should also call
	 * eventually DestroyComponent. (Might be playing an animation when this is called).
	 * UniqueID might get reset to null during this function.
	 * This does call ComponentStopped on the parent inventory component,
	 * but does NOT call ComponentFinished as a Component can be stopped if the logic failed,
	 * implying that the Component did not get to finish its logic.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NativeFunctions")
	void StopComponent(FGameplayTag StopResponse);

	/**Called by @StopComponent, this is where you would clean up loaded assets, perform
	 * final code, cancel animations, etc.
	 * This is called AFTER ComponentStopped delegate is called.*/
	UFUNCTION(BlueprintNativeEvent, Category = "NativeFunctions")
	void Cleanup();

	/**Announce to the owning inventory component that the logic behind this Component
	 * has finished.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NativeFunctions")
	void BroadcastItemComponentFinished(FGameplayTag FinishResponse);

	/**In case you have heavy assets that need to be loaded at a specific point.
	 * Usually it's safe to load assets during other functions, like the equip
	 * montage during the Equipped function.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Event")
	void LoadObjectAssets();

	virtual void DestroyComponent(bool bPromoteChildren = false) override;
};
