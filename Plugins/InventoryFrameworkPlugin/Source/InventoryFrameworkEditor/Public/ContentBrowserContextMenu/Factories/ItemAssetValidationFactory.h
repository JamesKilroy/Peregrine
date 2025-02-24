// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Objects/Parents/O_ItemAssetValidation.h"
#include "Factories/Factory.h"

#include "ItemAssetValidationFactory.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKEDITOR_API UItemAssetValidationFactory : public UFactory
{
	GENERATED_BODY()

public:

	UItemAssetValidationFactory(const FObjectInitializer& ObjectInitializer);

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

private:

	UPROPERTY()
	TSubclassOf<UO_ItemAssetValidation> ValidationClass;
};
