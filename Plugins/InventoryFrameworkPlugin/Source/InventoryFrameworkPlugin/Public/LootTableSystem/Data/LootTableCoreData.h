// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "LootTableCoreData.generated.h"


USTRUCT(BlueprintType)
struct FSimpleLootTable
{
	GENERATED_BODY()

	UPROPERTY(Category = "Loot Table", BlueprintReadWrite, EditAnywhere)
	UDA_CoreItem* Item = nullptr;

	UPROPERTY(Category = "Loot Table", BlueprintReadWrite, EditAnywhere)
	float SpawnPercentage = 1;

	UPROPERTY(Category = "Loot Table", BlueprintReadWrite, EditAnywhere)
	FIntPoint RandomMinMaxCount = FIntPoint(1, 1);
};
