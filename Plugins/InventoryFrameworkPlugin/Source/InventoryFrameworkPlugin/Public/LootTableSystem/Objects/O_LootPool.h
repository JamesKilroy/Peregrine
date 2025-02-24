// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "UObject/Object.h"
#include "O_LootPool.generated.h"

class UAC_LootTable;
class UAC_Inventory;

/**Loot pools are the objects that modify an inventory components
 * container settings before and after initialization, but are stored
 * in the loot table component.*/
UCLASS(Abstract, Blueprintable, DefaultToInstanced, EditInlineNew, CollapseCategories)
class INVENTORYFRAMEWORKPLUGIN_API UO_LootPool : public UObject
{
	GENERATED_BODY()

	virtual UWorld* GetWorld() const override;

public:

	/**How many items has this pool spawned?
	 * it is up to you to increment this.*/
	UPROPERTY(Category = "Loot Table", BlueprintReadWrite)
	int32 ItemsSpawned = 0;

	UFUNCTION(Category = "Loot Pool", BlueprintNativeEvent)
	void StartPlay();

	/**A property has been changed. This is only called in-editor.
	 * This can be used to redistribute spawn weights dynamically.
	 * If you are in C++, it's better to use PostEditChangeProperty,
	 * this is just exposing that function to Blueprint programmers.*/
	UFUNCTION(Category = "Loot Pool", BlueprintImplementableEvent)
	void PostPropertyChange(FName Property, FName MemberProperty);
	
	/**The loot table component has started processing any loot that might
	 * want to be added before the component is initialized.*/
	UFUNCTION(Category = "Loot Pool", BlueprintImplementableEvent)
	void ProcessPreInitializationLoot(UAC_Inventory* Inventory);

	/**The loot table component has started processing any loot that might
	 * want to be added after the component is initialized.*/
	UFUNCTION(Category = "Loot Pool", BlueprintImplementableEvent)
	void ProcessPostInitializationLoot(UAC_Inventory* Inventory);

	/**Start loading assets, such as any item assets this pool is referencing.
	 * By default, this is never called. You might want to call this when the
	 * player is in close proximity of the owner of this loot pool.*/
	UFUNCTION(Category = "Loot Pool", BlueprintNativeEvent)
	void PreLoadAssets();

	/**Get the number of items that this pool has spawned, optionally including
	 * the amount of all loot tables and the quantity of items they have spawned.
	 * This is useful for moments where you want to control how many items
	 * these loot pools can spawn.*/
	UFUNCTION(Category = "Loot Pool", BlueprintCallable, BlueprintPure)
	int32 GetNumberOfItemsAdded(bool IncludeOtherLootTables = true);

	UFUNCTION(Category = "Loot Pool", BlueprintCallable, BlueprintPure)
	UAC_Inventory* GetInventoryComponent();
	
	UFUNCTION(Category = "Loot Pool", BlueprintCallable, BlueprintPure)
    UAC_LootTable* GetLootTable();

	/**Add an @Item to the @Container. This will also import the containers
	 * and recursively add any loot tables from the @Item
	 * If you wish to add an item post initialization, use the standard
	 * TryAddNewItem function from the inventory component.*/
	UFUNCTION(Category = "Loot Pool", BlueprintCallable, DisplayName = "Add Item (Pre-initialize only)")
	void AddItemPreInitializeOnly(FS_InventoryItem Item, FS_ContainerSettings Container, bool IncludeLootTable = true);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
