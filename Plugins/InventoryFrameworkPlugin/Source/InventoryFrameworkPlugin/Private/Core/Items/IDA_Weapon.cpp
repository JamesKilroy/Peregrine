// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Items/IDA_Weapon.h"

#include "Core/Data/FL_InventoryFramework.h"


FText UIDA_Weapon::GetAssetTypeName()
{
	return FText(FText::FromString("Weapon"));
}

TSubclassOf<UW_AttachmentParent> UIDA_Weapon::GetAttachmentWidgetClass()
{
	return AttachmentWidget;
}

TArray<FS_ContainerSettings> UIDA_Weapon::GetDefaultContainers()
{
	return DefaultContainers;
}

TArray<UAC_LootTable*> UIDA_Weapon::GetLootTables()
{
	return LootTables;
}

bool UIDA_Weapon::CanItemTypeStack()
{
	return false;
}

#if WITH_EDITOR

void UIDA_Weapon::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
    
	if(!PropertyChangedEvent.Property->IsValidLowLevel())
	{
		return;
	}
    	
	FString StructName = PropertyChangedEvent.Property->GetOwnerStruct()->GetName();
	
	if(StructName == "S_InventoryItem" || StructName == "S_ItemOverwriteSettings")
	{
		UFL_InventoryFramework::ProcessContainerAndItemCustomizations(DefaultContainers);
	}
}

#endif