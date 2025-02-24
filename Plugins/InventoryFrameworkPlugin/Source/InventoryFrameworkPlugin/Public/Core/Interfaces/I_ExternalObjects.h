// Copyright (C) Varian Daemon 2023. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/Data/IFP_CoreData.h"
#include "Core/Fragments/IF_ItemOverrideSettings.h"
#include "I_ExternalObjects.generated.h"

class UW_InventoryItem;
UINTERFACE()
class UI_ExternalObjects : public UInterface
{
	GENERATED_BODY()
};

/**This interface is meant to be extended for your own needs.
 * These are basic updates meant for external widgets for either items
 * or containers to keep up to date for various updates.
 * This interface is very ambiguous and is designed to be extended.
 *
 * Most functions here are not meant to be called directly on a specific
 * object, they are meant to be called through their appropriate
 * "Broadcast" functions, which will gather all objects that are
 * listening for updates and send the update in a specific order.*/

class INVENTORYFRAMEWORKPLUGIN_API II_ExternalObjects
{
	GENERATED_BODY()

public:

	/**The destruct event for widgets can be called multiple times.
	 * When actors are destroyed, their pointer references are
	 * automatically removed, but this is not the case for widgets.
	 * When this is called, it is meant to behave similar to when
	 * an actor is destroyed; All references should be removed so
	 * this can be garbage collected properly.
	 * This is meant to be called BEFORE you call RemoveFromParent.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects", meta = (DeprecatedFunction = "Replaced with MarkObjectAsGarbage"))
	void RemoveWidgetReferences();

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	void ItemCountUpdated(FS_InventoryItem Item, int32 OldValue, int32 NewValue);
	

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	void LocationUpdated(FS_InventoryItem Item);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	void SizeUpdated(FS_InventoryItem Item);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	void RotationUpdated(FS_InventoryItem Item);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	void RarityUpdated(FS_InventoryItem Item);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	void ImageUpdated(FS_InventoryItem Item);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	void NameUpdated(FS_InventoryItem Item);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	void IconUpdated(UTexture* NewTexture, FS_InventoryItem Item);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	void AffordabilityUpdated(bool CanAfford, FS_InventoryItem Item);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	void ItemInstanceSpawned(FS_InventoryItem Item, UItemInstance* ItemInstance);

	/**If @Added is false, that means the tag was removed.
	 * Either @Item or @Container will be valid, not both at the same time,
	 * depending on if you are listening to an item or a container.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects")
	void TagsUpdated(FGameplayTag Tag, bool Added, FS_InventoryItem Item, FS_ContainerSettings ContainerSettings);

	/**If @Added is false, that means the tag was removed.
	 * Either @Item or @Container will be valid, not both at the same time,
	 * depending on if you are listening to an item or a container.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects")
	void TagValueUpdated(FS_TagValue TagValue, bool Added, float Delta, FS_InventoryItem Item, FS_ContainerSettings ContainerSettings);

	/**Item has been equipped, this is sent both to the item and the container
	 * it is equipping to or unequipped from.
	 * If @Equipped is false, it means the item was unequipped.
	 * @OldItemData In some cases, you need access to the new and old item data,
	 * for example when the item is being moved, the old item data will become
	 * invalid. This is a temporary copy of the item so you have access to that data.
	 * This can be invalid in some cases.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects")
	void ItemEquipStatusUpdate(FS_InventoryItem ItemData, bool Equipped, FS_InventoryItem OldItemData);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	void OverrideSettingsUpdated(FS_InventoryItem ItemData, FItemOverrideSettings OldOverride, FItemOverrideSettings NewOverride);

	/**Updates the background color. This can be similar to RarityUpdated,
	 * but allows for custom colors. This is primarily used for the InventoryHelper
	 * to show what item is currently selected.
	 *
	 * @IsTemporary This color will not get saved as the widgets current background color.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	void BackgroundColorUpdated(FS_InventoryItem Item, FLinearColor NewColor, bool IsTemporary = false);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	FLinearColor GetBackgroundColor();

	/**Has the user clicked on this widget and is trying to select it?
	 * Or have they selected another widget and its time to un-highlight
	 * this one?*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	void WidgetSelectionUpdated(FS_InventoryItem Item, bool IsSelected);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "IFP|External Objects|Items")
	void ItemDestroyed(FS_InventoryItem Item);
};

UCLASS()
class UFL_ExternalObjects : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastItemCountUpdated(FS_InventoryItem Item, int32 OldValue, int32 NewValue);

	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastLocationUpdated(FS_InventoryItem Item);

	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastSizeUpdated(FS_InventoryItem Item);

	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastRotationUpdated(FS_InventoryItem Item);

	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastRarityUpdated(FS_InventoryItem Item);

	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastImageUpdated(FS_InventoryItem Item);

	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastNameUpdated(FS_InventoryItem Item);

	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastIconUpdated(UTexture* NewTexture, FS_InventoryItem Item);

	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastAffordabilityUpdated(bool CanAfford, FS_InventoryItem Item);
	
	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastTagsUpdated(FGameplayTag Tag, bool Added, FS_InventoryItem Item, FS_ContainerSettings Container);

	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastTagValueUpdated(FS_TagValue TagValue, bool Added, float Delta, FS_InventoryItem Item, FS_ContainerSettings Container);

	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastOverrideSettingsUpdated(FS_InventoryItem Item, FItemOverrideSettings OldOverride, FItemOverrideSettings NewOverride);

	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastBackgroundColorUpdated(FS_InventoryItem Item, FLinearColor NewColor, bool IsTemporary = false);

	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastWidgetSelectionUpdated(FS_InventoryItem Item, bool IsSelected);

	/**Let everyone know about the equip status changing.
	 * @OldItemData This is only used when the item is being moved, because some equipment systems,
	 * like the default IFP equipment system, needs the original item data to fetch the meshes/actor
	 * that item created through the equipment system.
	 * @CustomTriggerFilters This is only used when the manual "AC_Inventory -> UpdateItemsEquipStatus" is called.
	 * Allows you to pass in custom trigger filters to use with equipment systems.*/
	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastItemEquipStatusUpdate(FS_InventoryItem Item, bool Equipped, TArray<FName> CustomTriggerFilters, FS_InventoryItem OldItemData = FS_InventoryItem());

	UFUNCTION(BlueprintCallable, Category = "IFP|External Objects")
	static void BroadcastItemInstanceSpawned(FS_InventoryItem Item, UItemInstance* ItemInstance);
};
