// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Core/Items/DA_CoreItem.h"
#include "Engine/DeveloperSettings.h"
#include "Subsystems/WorldSubsystem.h"
#include "TagLootPoolStorage.generated.h"

USTRUCT(BlueprintType)
struct FTagPoolItem
{
	GENERATED_BODY()

	UPROPERTY(Category = "Loot Pool", BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<UDA_CoreItem> Item;

	UPROPERTY(Category = "Loot Pool", BlueprintReadWrite, EditAnywhere)
	float SpawnChance = 1;

	UPROPERTY(Category = "Loot Table", BlueprintReadWrite, EditAnywhere)
	FIntPoint RandomMinMaxCount = FIntPoint(1, 1);
	
	FTagPoolItem()
	{
	}

	FTagPoolItem(TSoftObjectPtr<UDA_CoreItem> InItemPtr)
		: Item(InItemPtr)
	{
	}

	bool operator==(const FTagPoolItem& Argument) const
	{
		return Item == Argument.Item;
	}

	bool operator==(const TSoftObjectPtr<UDA_CoreItem>& Argument) const
	{
		return Item == Argument;
	}

	bool operator!=(const FTagPoolItem& Argument) const
	{
		return Item != Argument.Item;
	}

	bool operator!=(const TSoftObjectPtr<UDA_CoreItem>& Argument) const
	{
		return Item != Argument;
	}
};

USTRUCT(BlueprintType)
struct FTagLootPool
{
	GENERATED_BODY()

	UPROPERTY(Category = "Loot Pool", BlueprintReadWrite, EditAnywhere)
	TArray<FTagPoolItem> Items;

	TArray<TSoftObjectPtr<UDA_CoreItem>> GetItems()
	{
		TArray<TSoftObjectPtr<UDA_CoreItem>> FoundItems;
		for(auto& CurrentItem : Items)
		{
			if(!CurrentItem.Item.IsNull())
			{
				FoundItems.Add(CurrentItem.Item);
			}
		}
		return FoundItems;
	}
};

/**A subsystem for handling storing a simple map of tags and items,
 * to allow for loot tables to generate loot through a global, dynamic system
 * that can be modified by Modular Game Feature's or gameplay events.*/
UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UTagLootPoolStorage : public UWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UPROPERTY(Category = "Tag Loot Pool", BlueprintReadOnly)
	TMap<FGameplayTag, FTagLootPool> TagLootPools;

	UFUNCTION(Category = "Tag Loot Pool", BlueprintCallable, BlueprintPure)
	TArray<FTagPoolItem> GetItemsFromTagPool(FGameplayTag Tag);

	UFUNCTION(Category = "Tag Loot Pool", BlueprintCallable, BlueprintPure)
	float GetItemsSpawnChance(FGameplayTag TagPool, TSoftObjectPtr<UDA_CoreItem> Item);

	/**Add @Items to the specified loot pool assigned to the @TagPool
	 * If the item already exists, then the spawn chance and other properties
	 * passed in will simply get overriden with the new value.*/
	UFUNCTION(Category = "Tag Loot Pool", BlueprintCallable)
	void AddItemToTagLootPool(FGameplayTag TagPool, FTagPoolItem Item);

	/**Helper function for adding an entire TMap to the tag loot pool*/
	UFUNCTION(Category = "Tag Loot Pool", BlueprintCallable)
	void AppendTagLootPoolMap(TMap<FGameplayTag, FTagLootPool> TagPool);

	/**Remove @Items from the specified loot pool assigned to the @TagPool*/
	UFUNCTION(Category = "Tag Loot Pool", BlueprintCallable)
	void RemoveItemsFromTagLootPool(FGameplayTag TagPool, TArray<TSoftObjectPtr<UDA_CoreItem>> Items);

	/**Helper function for removing an entire TMap from the tag loot pool*/
	UFUNCTION(Category = "Tag Loot Pool", BlueprintCallable)
	void RemoveTagLootPoolMap(TMap<FGameplayTag, FTagLootPool> TagPool);
};
