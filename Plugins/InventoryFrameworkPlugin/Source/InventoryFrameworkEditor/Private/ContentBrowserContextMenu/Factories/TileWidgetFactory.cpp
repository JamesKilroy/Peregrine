// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "ContentBrowserContextMenu/Factories/TileWidgetFactory.h"
#include "DS_InventoryProjectSettings.h"
#include "WidgetBlueprint.h"
#include "Kismet2/KismetEditorUtilities.h"

class UDS_InventoryProjectSettings;
class FAssetToolsModule;

UTileWidgetFactory::UTileWidgetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UW_Tile::StaticClass();
	TileWidgetClass = SupportedClass;
}

bool UTileWidgetFactory::ConfigureProperties()
{
	TileWidgetClass = nullptr;
	if(const UDS_InventoryProjectSettings* IFPSettings = GetDefault<UDS_InventoryProjectSettings>())
	{
		TileWidgetClass = IFPSettings->TileWidget;
		return true;
	}

	//Object is null inside project settings, create the C++ parent
	TileWidgetClass = UW_Tile::StaticClass();

	return true;
}

UObject* UTileWidgetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	// Create new Blueprint
	UObject* NewObject = 
	FKismetEditorUtilities::CreateBlueprint(
		TileWidgetClass,
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
