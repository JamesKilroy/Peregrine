// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "ContentBrowserContextMenu/Factories/CraftingAssetFactory.h"
#include "ContentBrowserContextMenu/CraftingAssetCreationDialogue.h"


class FClassViewerModule;

#define LOCTEXT_NAMESPACE "InventoryFrameworkFactories"

UCraftingAssetFactory::UCraftingAssetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UDA_CoreCraftingRecipe::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

bool UCraftingAssetFactory::ConfigureProperties()
{
	TSharedRef<SCraftingAssetCreationDialogue> Dialogue = SNew(SCraftingAssetCreationDialogue);
	return Dialogue->ConfigureProperties(this);
}

UObject* UCraftingAssetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
                                            UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	if (DataAssetClass != nullptr)
	{
		return NewObject<UDA_CoreCraftingRecipe>(InParent, DataAssetClass, InName, Flags | RF_Transactional);
	}
	else
	{
		// if we have no data asset class, use the passed-in class instead
		check(InClass->IsChildOf(UDA_CoreCraftingRecipe::StaticClass()));
		return NewObject<UDA_CoreCraftingRecipe>(InParent, InClass, InName, Flags);
	}
}

#undef LOCTEXT_NAMESPACE
