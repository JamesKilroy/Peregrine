// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "ContentBrowserContextMenu/Factories/RecipeDisplayFactory.h"
#include "Kismet2/KismetEditorUtilities.h"

class UDS_InventoryProjectSettings;
class FAssetToolsModule;

URecipeDisplayFactory::URecipeDisplayFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UO_CoreRecipeDisplay::StaticClass();
	ObjectClass = SupportedClass;
}

bool URecipeDisplayFactory::ConfigureProperties()
{
	ObjectClass = UO_CoreRecipeDisplay::StaticClass();
	return true;
}

UObject* URecipeDisplayFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	// Create new Blueprint
	UObject* NewObject = 
	FKismetEditorUtilities::CreateBlueprint(
		ObjectClass,
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
