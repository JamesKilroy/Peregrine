// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "ContentBrowserContextMenu/Factories/ItemComponentFactory.h"
#include "ContentBrowserContextMenu/Factories/ItemComponentTraitFactory.h"
#include "DS_InventoryProjectSettings.h"
#include "Kismet2/KismetEditorUtilities.h"

class UDS_InventoryProjectSettings;
class FAssetToolsModule;

UItemComponentFactory::UItemComponentFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UItemComponent::StaticClass();
	ObjectClass = SupportedClass;
}

bool UItemComponentFactory::ConfigureProperties()
{
	ObjectClass = nullptr;
	if(const UDS_InventoryProjectSettings* IFPSettings = GetDefault<UDS_InventoryProjectSettings>())
	{
		if(IFPSettings->ItemTrait)
		{
			ObjectClass = IFPSettings->ItemComponent;
			return true;
		}
	}

	//Object is null inside project settings, create the C++ parent
	ObjectClass = UItemComponent::StaticClass();
	return true;
}

UObject* UItemComponentFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
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
