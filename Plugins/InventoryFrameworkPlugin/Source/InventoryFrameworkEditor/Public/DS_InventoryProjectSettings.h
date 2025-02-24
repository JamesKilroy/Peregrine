// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include <Core/Actors/Parents/A_ItemActor.h>
#include "Core/Traits/IT_ItemComponentTrait.h"
#include "Core/Traits/ItemTrait.h"
#include "Engine/DeveloperSettings.h"
#include "DS_InventoryProjectSettings.generated.h"

class UO_ItemAssetValidation;
class UW_Tile;
class UW_Container;
class UW_InventoryItem;
class UItemComponent;
class UIT_ItemComponentTrait;
class UItemTrait;

/**This class's primary use case is to allow people to customize what the content browser
 * context menu will create when making classes, but also allow people to reference blueprint
 * classes, as I imagine most people will want to keep their top-level parents in blueprints.*/

UCLASS(DisplayName = "Inventory Framework Plugin", DefaultConfig, Config = Editor)
class INVENTORYFRAMEWORKEDITOR_API UDS_InventoryProjectSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	
	UPROPERTY(Category = "Content Browser Context Menu", EditAnywhere, Config)
	TSubclassOf<AA_ItemActor> ItemActor = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/InventoryFrameworkPlugin/Core/ActorParents/BP_ItemActor.BP_ItemActor_C"));

	UPROPERTY(Category = "Content Browser Context Menu", EditAnywhere, Config, DisplayName = "Data-only Objects")
	TSubclassOf<UItemTrait> ItemTrait = UItemTrait::StaticClass();
	
	UPROPERTY(Category = "Content Browser Context Menu", EditAnywhere, Config)
	TSubclassOf<UIT_ItemComponentTrait> ItemComponentTrait = UIT_ItemComponentTrait::StaticClass();

	UPROPERTY(Category = "Content Browser Context Menu", EditAnywhere, Config)
	TSubclassOf<UItemComponent> ItemComponent = UItemComponent::StaticClass();

	UPROPERTY(Category = "Content Browser Context Menu", EditAnywhere, Config)
	TSubclassOf<UO_TagValueCalculation> TagValueCalculation = UO_TagValueCalculation::StaticClass();

	UPROPERTY(Category = "Content Browser Context Menu", EditAnywhere, Config)
	TSubclassOf<UW_AttachmentParent> AttachmentWidget = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/InventoryFrameworkPlugin/Core/Widgets/WBP_AttachmentParent.WBP_AttachmentParent_C"));
	
	UPROPERTY(Category = "Content Browser Context Menu", EditAnywhere, Config)
	TSubclassOf<UW_InventoryItem> ItemWidget = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/InventoryFrameworkPlugin/Core/Widgets/WBP_InventoryItem.WBP_InventoryItem_C"));

	UPROPERTY(Category = "Content Browser Context Menu", EditAnywhere, Config)
	TSubclassOf<UW_Container> ContainerWidget = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/InventoryFrameworkPlugin/Core/Widgets/WBP_Container.WBP_Container_C"));

	UPROPERTY(Category = "Content Browser Context Menu", EditAnywhere, Config)
	TSubclassOf<UW_Tile> TileWidget = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/InventoryFrameworkPlugin/Core/Widgets/WBP_Tile.WBP_Tile_C"));

	UPROPERTY(Category = "Content Browser Context Menu", EditAnywhere, Config)
	TSubclassOf<UO_ItemAssetValidation> ItemAssetValidation = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/InventoryFrameworkPlugin/Core/Editor/ItemAssetValidation/O_BasicItemValidation.O_BasicItemValidation_C"));

	/**How much to split the ContainerIdentifier tag in the details panel.
	 * For example: IFP.Player.Container.Weapons.Weapon1 will be turned into
	 * Weapons.Weapon1 if this is set to 2, if set to 3, it'll be turned into
	 * Container.Weapons.Weapon1*/
	UPROPERTY(Category = "Container Settings Detail Panel", EditAnywhere, Config)
	int32 TagHierarchySplitAmount = 0;

	/**Sometimes the dot in the gameplay tag string can make it hard to read.
	 * This lets you override the dot to be whatever you want.*/
	UPROPERTY(Category = "Container Settings Detail Panel", EditAnywhere, Config)
	FString TagSplitText = ".";

	/**Should the selectable items inside the @ItemAsset dropdown be filtered
	 * according to its parent container compability settings?
	 * For more info read tooltip for S_InventoryItem -> ItemAsset */
	UPROPERTY(Category = "Item Settings Detail Panel", EditAnywhere, Config)
	bool LimitItemSelectionToCompabilityCheck = true;

	virtual FName GetCategoryName() const override { return FName("Plugins"); }
	virtual FText GetSectionText() const override { return INVTEXT("Inventory Framework"); }
};
