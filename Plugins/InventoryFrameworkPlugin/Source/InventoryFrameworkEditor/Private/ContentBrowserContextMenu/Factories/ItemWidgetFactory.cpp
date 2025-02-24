// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "ContentBrowserContextMenu/Factories/ItemWidgetFactory.h"
#include "DS_InventoryProjectSettings.h"
#include "WidgetBlueprint.h"
#include "Kismet2/KismetEditorUtilities.h"

class UDS_InventoryProjectSettings;
class FAssetToolsModule;

UItemWidgetFactory::UItemWidgetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UW_InventoryItem::StaticClass();
	ItemWidgetClass = SupportedClass;
}

bool UItemWidgetFactory::ConfigureProperties()
{
	ItemWidgetClass = nullptr;
	if(const UDS_InventoryProjectSettings* IFPSettings = GetDefault<UDS_InventoryProjectSettings>())
	{
		ItemWidgetClass = IFPSettings->ItemWidget;
		return true;
	}

	//Object is null inside project settings, create the C++ parent
	ItemWidgetClass = UW_InventoryItem::StaticClass();

	return true;
}

UObject* UItemWidgetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	// Create new Blueprint
	UObject* NewObject = 
	FKismetEditorUtilities::CreateBlueprint(
		ItemWidgetClass,
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
