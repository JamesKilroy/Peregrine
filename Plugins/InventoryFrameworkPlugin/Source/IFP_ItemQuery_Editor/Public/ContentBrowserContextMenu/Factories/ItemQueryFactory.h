// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "O_ItemQueryBase.h"
#include "Factories/Factory.h"
#include "ItemQueryFactory.generated.h"

/**
 * 
 */
UCLASS()
class IFP_ITEMQUERY_EDITOR_API UItemQueryFactory : public UFactory
{
	GENERATED_BODY()

public:

	UItemQueryFactory(const FObjectInitializer& ObjectInitializer);

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

private:

	UPROPERTY()
	TSubclassOf<UO_ItemQueryBase> ObjectClass;
};
