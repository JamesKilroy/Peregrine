// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "ContentBrowserContextMenu/Factories/ItemActorFactory.h"
#include "DS_InventoryProjectSettings.h"
#include <Core/Actors/Parents/A_ItemActor.h>
#include "Kismet2/KismetEditorUtilities.h"

class UDS_InventoryProjectSettings;
class FAssetToolsModule;

UItemActorFactory::UItemActorFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = AA_ItemActor::StaticClass();
	ActorClass = SupportedClass;
}

bool UItemActorFactory::ConfigureProperties()
{
	static const FText TitleText = FText::FromString(TEXT("Pick Parent Item Class for new Interactable Component Object"));
	ActorClass = nullptr;

	UClass* ChosenClass = nullptr;
	if(const UDS_InventoryProjectSettings* IFPSettings = GetDefault<UDS_InventoryProjectSettings>())
	{
		ActorClass = IFPSettings->ItemActor;
		return true;
	}

	//Object is null inside project settings, create the C++ parent
	ActorClass = AA_ItemActor::StaticClass();

	return true;
}

UObject* UItemActorFactory::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags,
	UObject* Context, FFeedbackContext* Warn)
{
	// Create new Blueprint
	UObject* NewObject = 
	FKismetEditorUtilities::CreateBlueprint(
		ActorClass,
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
