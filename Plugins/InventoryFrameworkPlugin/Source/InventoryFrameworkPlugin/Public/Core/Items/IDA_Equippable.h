// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "Core/Items/DA_CoreItem.h"
#include "IDA_Equippable.generated.h"

/**Base class for items that can be equipped. This adds a widget
 * and default containers, allowing you to store that data in here
 * rather than the items actor.
 *
 * This is meant for jackets, backpacks, helmets and so forth.*/

UCLASS()
class INVENTORYFRAMEWORKPLUGIN_API UIDA_Equippable : public UDA_CoreItem
{
	GENERATED_BODY()

public:

	virtual FText GetAssetTypeName() override;
	
	//Attachment Settings
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Core Settings|Container Settings")
	TSubclassOf<class UW_AttachmentParent> AttachmentWidget;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Core Settings|Container Settings", meta=(TitleProperty = "ContainerIdentifier"))
	TArray<FS_ContainerSettings> DefaultContainers;

	/**Loot tables this item wants to include when it is created.
	 * These loot tables should only focus the containers that this item owns.
	 * Do note, the default behavior of StartComponent is that any inventory
	 * component that includes this item requires the tag
	 * IFP.Initialization.IncludeLootTables*/
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadWrite, Category = "Core Settings|Container Settings")
	TArray<UAC_LootTable*> LootTables;

	virtual TSubclassOf<UW_AttachmentParent> GetAttachmentWidgetClass() override;

	virtual TArray<FS_ContainerSettings> GetDefaultContainers() override;

	virtual TArray<UAC_LootTable*> GetLootTables() override;

	virtual bool CanItemTypeStack() override;
	
#if WITH_EDITOR
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

#endif
};
