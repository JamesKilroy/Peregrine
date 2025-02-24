// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/Data/IFP_CoreData.h"
#include "AC_LootTable.generated.h"

class UAC_Inventory;
class UO_LootPool;

/**The processor of loot pools, who's roles are to modify an inventories
 * containers before and after initialization to create a loot table system.*/
UCLASS(ClassGroup=(IFP), meta=(BlueprintSpawnableComponent), EditInlineNew)
class INVENTORYFRAMEWORKPLUGIN_API UAC_LootTable : public UActorComponent
{
	GENERATED_BODY()

public:
	
	UAC_LootTable();

	UPROPERTY(Category = "Loot Table", EditAnywhere, BlueprintReadWrite)
	int32 Priority = 0;
	
	UPROPERTY(Category = "Loot Table", Instanced, EditAnywhere, BlueprintReadWrite)
	TArray<UO_LootPool*> LootPools;

	/**Should this loot table destroy itself after it has performed its logic?
	 * This is recommended to have enabled, just in case that a loot table
	 * is hard referencing any assets, so they can be garbage collected properly.*/
	UPROPERTY(Category = "Loot Table", EditAnywhere, BlueprintReadWrite)
	bool DestroyAfterProcessing = true;

	/**Has this loot table finished its logic?
	 * You might want to save this value in your save system,
	 * so players can't keep re-rolling the loot pools by
	 * reloading their saves.*/
	UPROPERTY(Category = "Loot Table", BlueprintReadWrite, SaveGame)
	bool HasProcessedLootPools = false;

	UPROPERTY(Category = "Loot Table", BlueprintReadWrite, SaveGame)
	FS_UniqueID ItemID;

	virtual void BeginPlay() override;

	UFUNCTION(Category = "Loot Table", BlueprintPure, BlueprintCallable)
	UAC_Inventory* GetInventoryComponent();

	/**In case this loot table was created through the item
	 * data asset, this will give you the item that created
	 * this loot table.*/
	UFUNCTION(Category = "Loot Table", BlueprintPure, BlueprintCallable)
	FS_InventoryItem GetParentItem();

	/**In case this loot table was created through the item
	 * data asset, this will give you the containers that belong
	 * to the item that created this loot table.*/
	UFUNCTION(Category = "Loot Table", BlueprintPure, BlueprintCallable)
	TArray<FS_ContainerSettings> GetParentItemsContainers();

	/**Tell all loot pools to start async load any assets they might be referencing.*/
	UFUNCTION(Category = "Loot Table", BlueprintCallable)
	void PreLoadAssets();
	
	UFUNCTION(Category = "Loot Table", BlueprintNativeEvent)
	void PreInventoryInitialized(UAC_Inventory* Inventory);

	UFUNCTION(Category = "Loot Table", BlueprintNativeEvent)
	void PostInventoryInitialized(UAC_Inventory* Inventory);
};
