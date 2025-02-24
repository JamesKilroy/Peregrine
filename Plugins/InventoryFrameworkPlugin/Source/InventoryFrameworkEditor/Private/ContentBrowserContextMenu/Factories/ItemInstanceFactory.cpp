// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#include "ContentBrowserContextMenu/Factories/ItemInstanceFactory.h"
#include "ContentBrowserContextMenu/ItemInstanceCreationDialogue.h"
#include "Kismet2/KismetEditorUtilities.h"


class FClassViewerModule;

#define LOCTEXT_NAMESPACE "InventoryFrameworkFactories"

UItemInstanceFactory::UItemInstanceFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = UItemInstance::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

bool UItemInstanceFactory::ConfigureProperties()
{
	TSharedRef<SItemInstanceCreationDialogue> Dialogue = SNew(SItemInstanceCreationDialogue);
	return Dialogue->ConfigureProperties(this);
}

UObject* UItemInstanceFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
                                            UObject* Context, FFeedbackContext* Warn, FName CallingContext)
{
	// Create new Blueprint
	UObject* NewObject = 
	FKismetEditorUtilities::CreateBlueprint(
		ItemInstanceClass,
		InParent,
		InName,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		NAME_None
	);

	NewObject->Modify(true);
	return NewObject;
}

#undef LOCTEXT_NAMESPACE
