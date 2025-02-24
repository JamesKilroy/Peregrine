// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Core/Components/ItemComponent.h"
#include "ItemComponentFactory.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKEDITOR_API UItemComponentFactory : public UFactory
{
	GENERATED_BODY()

public:

	UItemComponentFactory(const FObjectInitializer& ObjectInitializer);

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

private:

	UPROPERTY()
	TSubclassOf<UItemComponent> ObjectClass;
};
