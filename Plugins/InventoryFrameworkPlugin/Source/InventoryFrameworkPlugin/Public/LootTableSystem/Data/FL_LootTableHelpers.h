// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "FL_LootTableHelpers.generated.h"

class UAC_LootTable;
/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UFL_LootTableHelpers : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(Category = "Loot Table Helpers", BlueprintCallable)
	static TArray<UAC_LootTable*> GetLootTableComponents(AActor* Actor, bool IgnorePriority = false);

	UFUNCTION(Category = "Loot Table Helpers", BlueprintCallable)
	static TArray <UAC_LootTable*> SortLootTablesByPriority(TArray <UAC_LootTable*> ArrayToSort);
};
