// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "DS_InventoryProjectSettings.h"
#include "ContentBrowserContextMenu/Factories/ItemAssetValidationFactory.h"
#include "Kismet2/KismetEditorUtilities.h"

class UDS_InventoryProjectSettings;
class FAssetToolsModule;

UItemAssetValidationFactory::UItemAssetValidationFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UO_ItemAssetValidation::StaticClass();
	ValidationClass = SupportedClass;
}

bool UItemAssetValidationFactory::ConfigureProperties()
{
	ValidationClass = nullptr;
	if(const UDS_InventoryProjectSettings* IFPSettings = GetDefault<UDS_InventoryProjectSettings>())
	{
		ValidationClass = IFPSettings->ItemAssetValidation;
		return true;
	}

	//Object is null inside project settings, create the C++ parent
	ValidationClass = UO_ItemAssetValidation::StaticClass();

	return true;
}

UObject* UItemAssetValidationFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	// Create new Blueprint
	UObject* NewObject = 
	FKismetEditorUtilities::CreateBlueprint(
		ValidationClass,
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
