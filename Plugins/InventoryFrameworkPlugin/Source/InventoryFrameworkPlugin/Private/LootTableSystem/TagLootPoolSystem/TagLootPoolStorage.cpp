// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "LootTableSystem/TagLootPoolSystem/TagLootPoolStorage.h"

#include "GameFeatureData.h"
#include "GameFeaturesSubsystem.h"
#include "GameplayTagContainer.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LootTableSystem/TagLootPoolSystem/GFA_AddItemsToTagLootPool.h"


bool UTagLootPoolStorage::ShouldCreateSubsystem(UObject* Outer) const
{
	//This subsystem should not exist on clients, loot tables do not work on clients anyways
	return UKismetSystemLibrary::IsServer(Outer);
}

void UTagLootPoolStorage::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	//Get all active game features when the world is loaded, so we can populate the tag loot pool
	UGameFeaturesSubsystem* GameFeaturesSubsystem = &UGameFeaturesSubsystem::Get();
	TArray<const UGameFeatureData*> OutActivePluginFeatureData;
	GameFeaturesSubsystem->GetGameFeatureDataForActivePlugins(OutActivePluginFeatureData);
	
	for(auto& CurrentGameFeature : OutActivePluginFeatureData)
	{
		for(auto& CurrentAction : CurrentGameFeature->GetActions())
		{
			if(UGFA_AddItemsToTagLootPool* TagLootPool = Cast<UGFA_AddItemsToTagLootPool>(CurrentAction))
			{
				AppendTagLootPoolMap(TagLootPool->TagLootPools);
			}
		}
	}
}

TArray<FTagPoolItem> UTagLootPoolStorage::GetItemsFromTagPool(FGameplayTag Tag)
{
	if(FTagLootPool* TagLootPool = TagLootPools.Find(Tag))
	{
		return TagLootPool->Items;
	}

	return TArray<FTagPoolItem>();
}

float UTagLootPoolStorage::GetItemsSpawnChance(FGameplayTag TagPool, TSoftObjectPtr<UDA_CoreItem> Item)
{
	if(FTagLootPool* TagLootPool = TagLootPools.Find(TagPool))
	{
		for(auto& CurrentItem : TagLootPool->Items)
		{
			if(CurrentItem.Item == Item)
			{
				return CurrentItem.SpawnChance;
			}
		}
	}

	return 0;
}

void UTagLootPoolStorage::AddItemToTagLootPool(FGameplayTag TagPool, FTagPoolItem Item)
{
	FTagLootPool* TagLootPool = TagLootPools.Find(TagPool);
	if(!TagLootPool)
	{
		FTagLootPool NewLootPool;
		NewLootPool.Items.Add(FTagPoolItem(Item));
		TagLootPools.Add(TagPool, NewLootPool);
		return;
	}

	for(auto& CurrentItem : TagLootPool->Items)
	{
		if(CurrentItem.Item == Item.Item)
		{
			/**While I could just set the spawn chance,
			 * I am overriding the item struct in its entirety
			 * because people might expand that struct to inlucde
			 * other properties.*/
			CurrentItem = Item;
			return;
		}
	}
	
	TagLootPool->Items.AddUnique(FTagPoolItem(Item));
}

void UTagLootPoolStorage::AppendTagLootPoolMap(TMap<FGameplayTag, FTagLootPool> TagPool)
{
	for(auto& CurrentTag : TagPool)
	{
		for(auto& CurrentItem : CurrentTag.Value.Items)
		{
			AddItemToTagLootPool(CurrentTag.Key, CurrentItem);
		}
	}
}

void UTagLootPoolStorage::RemoveItemsFromTagLootPool(FGameplayTag TagPool, TArray<TSoftObjectPtr<UDA_CoreItem>> Items)
{
	FTagLootPool* TagLootPool = TagLootPools.Find(TagPool);
	if(!TagLootPool)
	{
		return;
	}
	
	for(auto& CurrentItem : Items)
	{
		TagLootPool->Items.RemoveSingle(CurrentItem);
		if(TagLootPool->Items.IsEmpty())
		{
			TagLootPools.Remove(TagPool);
			return;
		}
	}
}

void UTagLootPoolStorage::RemoveTagLootPoolMap(TMap<FGameplayTag, FTagLootPool> TagPool)
{
	for(auto& CurrentTag : TagPool)
	{
		RemoveItemsFromTagLootPool(CurrentTag.Key, CurrentTag.Value.GetItems());
	}
}
