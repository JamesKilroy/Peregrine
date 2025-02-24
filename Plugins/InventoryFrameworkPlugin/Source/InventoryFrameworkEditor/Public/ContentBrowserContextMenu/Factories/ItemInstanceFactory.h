// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Objects/Parents/ItemInstance.h"
#include "Factories/Factory.h"
#include "ItemInstanceFactory.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKEDITOR_API UItemInstanceFactory : public UFactory
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=DataAsset)
	TSubclassOf<UItemInstance> ItemInstanceClass;

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn, FName CallingContext) override;
};
