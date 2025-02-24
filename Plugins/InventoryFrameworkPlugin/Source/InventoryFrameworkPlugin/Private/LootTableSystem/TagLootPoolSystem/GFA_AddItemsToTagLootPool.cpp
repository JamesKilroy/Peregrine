// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "LootTableSystem/TagLootPoolSystem/GFA_AddItemsToTagLootPool.h"

#include "GameFeaturesSubsystem.h"

void UGFA_AddItemsToTagLootPool::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	Super::OnGameFeatureActivating(Context);

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if(Context.ShouldApplyToWorldContext(WorldContext))
		{
			UTagLootPoolStorage* TagLootPoolStorage = WorldContext.World()->GetSubsystem<UTagLootPoolStorage>();
			if(!TagLootPoolStorage)
			{
				return;
			}
			
			TagLootPoolStorage->AppendTagLootPoolMap(TagLootPools);
		}
	}
}

void UGFA_AddItemsToTagLootPool::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	Super::OnGameFeatureDeactivating(Context);

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if(Context.ShouldApplyToWorldContext(WorldContext))
		{
			UTagLootPoolStorage* TagLootPoolStorage = WorldContext.World()->GetSubsystem<UTagLootPoolStorage>();
			if(!TagLootPoolStorage)
			{
				return;
			}
			
			TagLootPoolStorage->RemoveTagLootPoolMap(TagLootPools);
		}
	}
}