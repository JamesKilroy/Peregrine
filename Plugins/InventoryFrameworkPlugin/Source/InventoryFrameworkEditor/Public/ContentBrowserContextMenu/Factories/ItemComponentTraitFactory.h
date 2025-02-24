// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Traits/IT_ItemComponentTrait.h"
#include "Factories/Factory.h"
#include "ItemComponentTraitFactory.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKEDITOR_API UItemComponentTraitFactory : public UFactory
{
	GENERATED_BODY()

public:

	UItemComponentTraitFactory(const FObjectInitializer& ObjectInitializer);

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

private:

	UPROPERTY()
	TSubclassOf<UIT_ItemComponentTrait> ObjectClass;
};
