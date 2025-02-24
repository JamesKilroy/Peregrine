// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Items/IDA_Equippable.h"

#include "Core/Data/FL_InventoryFramework.h"


FText UIDA_Equippable::GetAssetTypeName()
{
	return FText(FText::FromString("Equippable"));
}

TSubclassOf<UW_AttachmentParent> UIDA_Equippable::GetAttachmentWidgetClass()
{
	return AttachmentWidget;
}

TArray<FS_ContainerSettings> UIDA_Equippable::GetDefaultContainers()
{
	return DefaultContainers;
}

TArray<UAC_LootTable*> UIDA_Equippable::GetLootTables()
{
	return LootTables;
}

bool UIDA_Equippable::CanItemTypeStack()
{
	return false;
}

#if WITH_EDITOR

void UIDA_Equippable::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if(!PropertyChangedEvent.Property->IsValidLowLevel())
	{
		return;
	}
	
	FString StructName = PropertyChangedEvent.Property->GetOwnerStruct()->GetName();
	
	const FString PropertyName = PropertyChangedEvent.GetPropertyName().ToString();
	if(PropertyName == "Item" || StructName == "S_ItemOverwriteSettings")
	{
		UFL_InventoryFramework::ProcessContainerAndItemCustomizations(DefaultContainers);
	}
}

#endif

