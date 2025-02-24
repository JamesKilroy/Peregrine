// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "GameplayTagContainer.h"
#include "TagLootPoolStorage.h"

#include "GFA_AddItemsToTagLootPool.generated.h"


/**
 * Adds a set of items to a specific tag loot pool.
 */
UCLASS(DisplayName = "Add Items to tag loot pool")
class INVENTORYFRAMEWORKPLUGIN_API UGFA_AddItemsToTagLootPool : public UGameFeatureAction
{
	GENERATED_BODY()

public:

	/**Items to add to the world's global tag loot pools.*/
	UPROPERTY(Category = "Tag Loot Pool", EditAnywhere, meta = (ForceInlineRow))
	TMap<FGameplayTag, FTagLootPool> TagLootPools;

	UPROPERTY()
	TObjectPtr<UTagLootPoolStorage> TagLootPoolSubsystem = nullptr;

	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;

	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
};
