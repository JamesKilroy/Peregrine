// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "ContentBrowserContextMenu/Factories/AttachmentWidgetFactory.h"
#include "DS_InventoryProjectSettings.h"
#include "WidgetBlueprint.h"
#include "Core/Widgets/W_AttachmentParent.h"
#include "Kismet2/KismetEditorUtilities.h"

class UDS_InventoryProjectSettings;
class FAssetToolsModule;

UAttachmentWidgetFactory::UAttachmentWidgetFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UW_AttachmentParent::StaticClass();
	AttachmentWidgetClass = SupportedClass;
}

bool UAttachmentWidgetFactory::ConfigureProperties()
{
	AttachmentWidgetClass = nullptr;
	if(const UDS_InventoryProjectSettings* IFPSettings = GetDefault<UDS_InventoryProjectSettings>())
	{
		AttachmentWidgetClass = IFPSettings->AttachmentWidget;
		return true;
	}

	//Object is null inside project settings, create the C++ parent
	AttachmentWidgetClass = UW_AttachmentParent::StaticClass();

	return true;
}

UObject* UAttachmentWidgetFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	// Create new Blueprint
	UObject* NewObject = 
	FKismetEditorUtilities::CreateBlueprint(
		AttachmentWidgetClass,
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
