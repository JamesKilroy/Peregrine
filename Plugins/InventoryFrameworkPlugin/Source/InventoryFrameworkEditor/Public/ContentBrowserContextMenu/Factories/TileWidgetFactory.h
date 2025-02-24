// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"
#include "Core/Widgets/W_Tile.h"
#include "TileWidgetFactory.generated.h"

/**
 * 
 */
UCLASS()
class INVENTORYFRAMEWORKEDITOR_API UTileWidgetFactory : public UFactory
{
	GENERATED_BODY()

public:

	UTileWidgetFactory(const FObjectInitializer& ObjectInitializer);

	virtual bool ConfigureProperties() override;
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;

private:

	UPROPERTY()
	TSubclassOf<UW_Tile> TileWidgetClass;
};
