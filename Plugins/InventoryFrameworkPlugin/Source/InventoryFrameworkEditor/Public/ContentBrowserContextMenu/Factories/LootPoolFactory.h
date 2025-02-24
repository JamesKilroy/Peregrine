// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "Factories/Factory.h"
#include "LootTableSystem/Objects/O_LootPool.h"
#include "LootPoolFactory.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKEDITOR_API ULootPoolFactory : public UFactory
{
	GENERATED_BODY()

public:

	ULootPoolFactory(const FObjectInitializer& ObjectInitializer);
	
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

private:

	UPROPERTY()
	TSubclassOf<UO_LootPool> ObjectClass;
};
