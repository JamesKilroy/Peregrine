// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "ContentBrowserContextMenu/Factories/ContainerWidgetFactory.h"
#include "DS_InventoryProjectSettings.h"
#include "WidgetBlueprint.h"
#include "Kismet2/KismetEditorUtilities.h"

class UDS_InventoryProjectSettings;
class FAssetToolsModule;

UContainerWidgetFactory::UContainerWidgetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UW_Container::StaticClass();
	ContainerWidgetClass = SupportedClass;
}

bool UContainerWidgetFactory::ConfigureProperties()
{
	ContainerWidgetClass = nullptr;
	if(const UDS_InventoryProjectSettings* IFPSettings = GetDefault<UDS_InventoryProjectSettings>())
	{
		ContainerWidgetClass = IFPSettings->ContainerWidget;
		return true;
	}

	//Object is null inside project settings, create the C++ parent
	ContainerWidgetClass = UW_Container::StaticClass();

	return true;
}

UObject* UContainerWidgetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	// Create new Blueprint
	UObject* NewObject = 
	FKismetEditorUtilities::CreateBlueprint(
		ContainerWidgetClass,
		InParent,
		InName,
		BPTYPE_Normal,
		UWidgetBlueprint::StaticClass(),
		UWidgetBlueprintGeneratedClass::StaticClass(),
		NAME_None
	);

	NewObject->Modify(true);
	return NewObject;
}
