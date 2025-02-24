// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "ItemTrait.h"
#include "Core/Data/IFP_CoreData.h"
#include "IT_ItemComponentTrait.generated.h"


UENUM(BlueprintType)
enum EConstructionPolicy
{
	PerActor,
	PerUse,
	Custom
};

/**This class is an extension of UItemTrait and is meant to be
 * associated with an item component to handle logic tied to an item.
 * This has several benefits, but is more expensive.
 * 1. The item Component will be viable for replication.
 * 2. Instanced UObjects are known to be very buggy whenever you try and
 * modify the data inside of them. Since these methods construct a whole new
 * item component, we can get around that buggy behavior.
 * 3. Timers, delays, animations, async tasks and RPC's will work.
 *
 * You can continue to run basic functions inside this object, but they should always be
 * instant and not modify the variables living on this object.
 *
 * Docs:
 * https://inventoryframework.github.io/classes-and-settings/traitsandcomponents */

UCLASS(Abstract, Blueprintable, meta=(ShowWorldContextPin), EditInlineNew, DefaultToInstanced, HideCategories = ("DoNotShow", "Default"))
class INVENTORYFRAMEWORKPLUGIN_API UIT_ItemComponentTrait : public UItemTrait
{
	GENERATED_BODY()

public:
	UIT_ItemComponentTrait();
	
	//--------------------
	// Variables

	/**@Both Item Component will get constructed on the server and replicated to the client.
	 * If you're working on a multiplayer project, this is the most common networking
	 * method you'll be using.
	 * 
	 *@Server Item Component will only exist on the server. Only the server is allowed
	 * to construct this Component. This is the safest method but biggest nuisance
	 * to execute.
	 *
	 * @Client Item Component will only exist on clients, replication will not happen
	 * unless the client is the listen server. This is the least secure and will
	 * work virtually every time.
	 *
	 * For single player games, none of these options will change their behavior. They
	 * will all work, but it is recommended to keep them set to @Both in case you ever
	 * want to try out multiplayer.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Settings")
	TEnumAsByte<EObjectNetworkingMethod> NetworkingMethod = Both;

	/**@PerActor Only 1 copy of the ItemComponent is allowed per item.
	 * Example: You have two stacks of apples, you are allowed to use
	 * stack 1 once until its logic destroys the component. But you are
	 * allowed to use Stack 2 while Stack 1 is still finishing.
	 *
	 * @PerUse A new ItemComponent will get created per use of this Object.
	 * 
	 * @Custom Whenever a new copy of the item Component has a chance to
	 * spawn, it'll ask AllowNewInstance. Override this function inside
	 * this UObject's class.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Settings")
	TEnumAsByte<EConstructionPolicy> ConstructionPolicy = PerActor;

	/**The ItemComponent that will be spawned to handle the gameplay logic.
	 * This object can run timers, async tasks, RPC's and more.
	 * This is just an actor component that is attached to the owner of the
	 * item, but has special logic and functions to retrieve the item
	 * this ItemComponent belongs to.
	 * Since a unique instance is being made, all variables on this
	 * ItemComponent are safe to modify during runtime. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Settings")
	TSoftClassPtr<UItemComponent> ItemComponent = nullptr;

	/**Should this Component follow the item struct, where ever it goes?
	 * This could be useful if you are storing data, such as ammo in a Component
	 * and want it to follow the item. Whenever the owning item is moved to a
	 * new actor, the item Component will get removed from the old actor
	 * and re-attached to the new actor, preserving all the data.
	 * NOTE: For simple floats, it is suggested to use the TagValue system
	 * as it is much more efficient and easier to handle.
	 * This might not work for all Components.*/
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Settings")
	bool FollowItem = false;

	UPROPERTY(BlueprintReadWrite, meta=(ExposeOnSpawn=true), Category = "Settings")
	FS_UniqueID UniqueID;
	

	//--------------------
	// Functions

	/**Called when @ConstructionPolicy is set to Custom.*/
	UFUNCTION(BlueprintImplementableEvent, Category = "Construction")
	bool AllowNewInstance();

	virtual TArray<FString> VerifyData_Implementation(UDA_CoreItem* ItemAsset) override;
};
