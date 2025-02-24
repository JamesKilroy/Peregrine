// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ActorFactories/ActorFactoryBlueprint.h"
#include <Core/Actors/Parents/A_ItemActor.h>
#include "Factories/BlueprintFactory.h"
#include "Factories/Factory.h"
#include "ItemActorFactory.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKEDITOR_API UItemActorFactory : public UFactory
{
	GENERATED_BODY()

public:

	UItemActorFactory(const FObjectInitializer& ObjectInitializer);

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

private:

	UPROPERTY()
	TSubclassOf<AA_ItemActor> ActorClass;
};
