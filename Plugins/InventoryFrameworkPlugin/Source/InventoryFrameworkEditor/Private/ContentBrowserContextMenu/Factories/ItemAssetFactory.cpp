// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "ContentBrowserContextMenu/Factories/ItemAssetFactory.h"
#include "ContentBrowserContextMenu/ItemCreationDialogue.h"
#include "Core/Items/DA_CoreItem.h"


class FClassViewerModule;

#define LOCTEXT_NAMESPACE "InventoryFrameworkFactories"

UItemFactory::UItemFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UDA_CoreItem::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

bool UItemFactory::ConfigureProperties()
{
	TSharedRef<SItemCreationDialogue> Dialogue = SNew(SItemCreationDialogue);
	return Dialogue->ConfigureProperties(this);
}

UObject* UItemFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
                                            UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	if (DataAssetClass != nullptr)
	{
		return NewObject<UDA_CoreItem>(InParent, DataAssetClass, InName, Flags | RF_Transactional);
	}
	else
	{
		// if we have no data asset class, use the passed-in class instead
		check(InClass->IsChildOf(UDA_CoreItem::StaticClass()));
		return NewObject<UDA_CoreItem>(InParent, InClass, InName, Flags);
	}
}

#undef LOCTEXT_NAMESPACE
