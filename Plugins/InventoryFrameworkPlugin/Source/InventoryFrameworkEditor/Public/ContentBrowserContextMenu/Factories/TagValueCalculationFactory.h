// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <Core/Actors/Parents/A_ItemActor.h>
#include "Factories/Factory.h"
#include "TagValueCalculationFactory.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKEDITOR_API UTagValueCalculationFactory : public UFactory
{
	GENERATED_BODY()

public:

	UTagValueCalculationFactory(const FObjectInitializer& ObjectInitializer);

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

private:

	UPROPERTY()
	TSubclassOf<UO_TagValueCalculation> ObjectClass;
};
