// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Traits/ItemTrait.h"
#include "Factories/Factory.h"
#include "DataObjectFactory.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKEDITOR_API UDataObjectFactory : public UFactory
{
	GENERATED_BODY()

public:

	UDataObjectFactory(const FObjectInitializer& ObjectInitializer);

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

private:

	UPROPERTY()
	TSubclassOf<UItemTrait> ObjectClass;
};
