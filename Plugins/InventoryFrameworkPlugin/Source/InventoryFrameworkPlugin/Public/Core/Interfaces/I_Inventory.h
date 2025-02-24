// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Core/Data/IFP_CoreData.h"
#include "I_Inventory.generated.h"

class UAC_Inventory;
class UW_Container;


// This class does not need to be modified.
UINTERFACE(Blueprintable, MinimalAPI)
class UI_Inventory : public UInterface
{
	GENERATED_BODY()
};


class INVENTORYFRAMEWORKPLUGIN_API II_Inventory
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	void GetInventoryComponent(UAC_Inventory*& Component);

	/**AC_Inventory -> CanAffordItem will call this to see if any designers have implemented their
	 * own custom logic for handling currency exchanges between vendors and players.
	 * @Buyer Refers to the component that is attempting to purchase the item.
	 * @Currency The item type of currency the buyer is attempting to use to purchase the item.
	 * @Amount The amount the buyer is trying to use of the proposed currency to purchase the item.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	bool MeetsCurrencyCheck(UAC_Inventory* Buyer, UIDA_Currency* Currency, int32 Amount);

	/**Called when a child of BP_SM_ItemPhysical is spawned, this will pass the UniqueID
	 * the item struct has so the spawned actor can retrieve its true item data properly.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	void PassUniqueID(FS_UniqueID UniqueID);

	/**Called by the equipment manager whenever an actor or component is
	 * spawned/revealed/attached to a new socket.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	void ItemEquipped(USceneComponent* Component, AActor* Actor, FS_UniqueID ItemID);

	/**Called by the equipment manager whenever an actor or component is
	 * destroyed/hidden/attached to a new socket*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory")
	void ItemUnequipped(USceneComponent* Component, AActor* Actor, FS_UniqueID ItemID, ERemovalPolicy RemovalPolicy);

	/**Label this actor as a preview actor instance, this can simply be a boolean
	 * that is then returned in the @IsPreviewActor function.
	 * @OriginalActor might not be valid in all scenarios.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|Preview Actor")
	void SetIsPreviewActor(bool NewStatus, AActor* OriginalActor);

	/**Ask this actor if they are a real actor or just a preview actor.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|Preview Actor")
	bool IsPreviewActor();
};
