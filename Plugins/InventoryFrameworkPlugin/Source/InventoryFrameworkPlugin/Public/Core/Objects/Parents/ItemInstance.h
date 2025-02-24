// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "O_NetworkedObject.h"
#include "Core/Data/IFP_CoreData.h"
#include "Core/Interfaces/I_ExternalObjects.h"
#include "UObject/Object.h"
#include "ItemInstance.generated.h"

/**Most inventory systems use either a struct or an object to represent
 * their items. In IFP, the method used is a struct with an optional
 * object that is tied to the struct.
 * This object can run gameplay logic, have any variables added and removed,
 * manage delegates, perform delays, load assets, run RPC's and so forth.
 * 
 * The item will still function completely as normally without this object.
 * This object is meant to be an extension of the item struct.
 *
 * Item Instances receive the same events that external objects get
 * through the I_ExternalObjects interface.
 *
 * Because of how tied this class is to the item asset, their documentation
 * is tied together:
 * https://inventoryframework.github.io/classes-and-settings/da_coreitem/#item-struct-object */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, HideCategories = ("DoNotShow", "Default"))
class INVENTORYFRAMEWORKPLUGIN_API UItemInstance : public UO_NetworkedObject, public II_ExternalObjects
{
	GENERATED_BODY()

public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void StartPlay_Implementation() override;

	/**The ID of the item this object belongs to.
	 * Used to fetch the ItemData of the item this object
	 * belongs to.*/
	UPROPERTY(Category = "Item Data", BlueprintReadWrite, ReplicatedUsing = "OnRep_ItemID")
	FS_UniqueID ItemID;
	UFUNCTION()
	void OnRep_ItemID(FS_UniqueID OldUniqueID);

	//Called when the ItemID
	UFUNCTION(Category = "Item Instance", BlueprintNativeEvent, DisplayName = "ItemID Updated")
	void ItemIDUpdated(FS_UniqueID OldUniqueID);

	/**The item asset this object was created from.*/
	UPROPERTY(Category = "Item Data", BlueprintReadOnly, SaveGame)
	UDA_CoreItem* ItemAsset = nullptr;

	/**If checked to false, this object is created when the item struct is created.
	 * If true, it is only constructed when some code calls GetItemsInstance
	 * and no object for the item has been created. IE, constructing this object
	 * when requested for usage.*/
	UPROPERTY(Category = "Settings", BlueprintReadOnly, EditAnywhere, SaveGame)
	bool ConstructOnRequest = true;

	UPROPERTY(Category = "Settings", BlueprintReadOnly, EditAnywhere, SaveGame)
	TEnumAsByte<ELifetimeCondition> ReplicationCondition;

	UFUNCTION(Category = "Item Data", BlueprintCallable, BlueprintPure, meta = (CompactNodeTitle = "Inventory"))
	UAC_Inventory* GetInventoryComponent();
	
	UFUNCTION(Category = "Item Data", BlueprintCallable, BlueprintPure, meta = (CompactNodeTitle = "Item Data"))
	FS_InventoryItem GetItemData();

	/**Gets the container the owning item is inside of.*/
	UFUNCTION(Category = "Item Data", BlueprintCallable, BlueprintPure, meta = (CompactNodeTitle = "Container Settings"))
	FS_ContainerSettings GetContainerSettings();

	virtual void RemoveObject() override;

	virtual AActor* GetOwningActor() const override;
};
