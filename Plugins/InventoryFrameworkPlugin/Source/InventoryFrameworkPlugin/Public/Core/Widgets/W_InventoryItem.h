// Copyright (C) Varian Daemon 2023. All Rights Reserved.

// Docs: https://inventoryframework.github.io/classes-and-settings/w_inventoryitem/


#pragma once

#include "CoreMinimal.h"
#include "W_Tile.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/UserWidget.h"
#include "Components/CircularThrobber.h"
#include "Components/Image.h"
#include "Core/Interfaces/I_ExternalObjects.h"
#include "W_InventoryItem.generated.h"

class UW_Tile;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemCountChanged, int32, OldCount, int32, NewCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOpenContextMenu, UW_InventoryItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FItemDragged, UW_InventoryItem*, Item, UDragDropOperation*, DragOperation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FItemDragEnded, UW_InventoryItem*, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FItemPressed, UW_InventoryItem*, Item);


/**Base widget to represent items.*/
UCLASS(Abstract)
class INVENTORYFRAMEWORKPLUGIN_API UW_InventoryItem : public UUserWidget, public II_ExternalObjects
{
	GENERATED_BODY()

public:

	//--------------------
	// Variables

	/**Used to get the direct item struct from the component.
	 * You could make a ContainerArrayIndex to further optimize GetItemData.
	 * but that would require you to keep that up to date for when you remove, add
	 * or sort containers.*/
	UPROPERTY(BlueprintReadWrite, Category = "Settings", meta=(ExposeOnSpawn="true"))
	int32 ItemsArrayIndex;

	UPROPERTY(BlueprintReadWrite, Category = "Settings", meta=(ExposeOnSpawn="true"))
	FS_UniqueID ItemID;

	UPROPERTY(BlueprintReadWrite, Category = "Settings", meta=(ExposeOnSpawn="true"))
	FS_UniqueID ContainerID;
	
	UPROPERTY(BlueprintReadWrite, Category = "Tiles", meta = (DeprecatedProperty, DeprecationMessage = "Nobody was using ParentTiles, was just taking up memory. Use GetOwningTiles instead"))
	TArray<UW_Tile*> ParentTiles;

	UPROPERTY(BlueprintReadWrite, Category = "Container")
	UW_Container* ParentContainer;

	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	float OpacityTarget = 1.0;

	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	float ScaleTarget = 1.0;

	/**When the player clicks on the item, which background tile is being clicked?
	 * This gives a local offset, so for example; If the item is 3x3, and you click
	 * the top right tile, it'll give you the SizeOfWidget * X and SizeOfWidget * Y.
	 * This is only important for items that are in a grid container.*/
	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	FIntPoint CurrentHoveredTile;

	/**Sometimes you need a widget for an item, but you are either in-editor
	 * or wish to display a widget without initializing the item
	 * This case should be rare
	 * If the item is initialized, this should be empty.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings", meta=(ExposeOnSpawn="true"))
	FS_InventoryItem RawItemData;

	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	bool IsDragging = false;

	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	FLinearColor CurrentBackgroundColor;

	UPROPERTY(BlueprintReadWrite, Category = "Settings")
	bool IsHighlighted = false;

	//--------------------
	// Delegates
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemCountChanged ItemCountChanged;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FOpenContextMenu OpenContextMenu;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemDragged ItemDragged;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemDragEnded ItemDragEnded;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FItemPressed ItemPressed;

	//--------------------
	// Functions

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, BlueprintPure, Category = "Design", meta = (CompactNodeTitle = "Size Box"))
	USizeBox* GetSizeBox();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, BlueprintPure, Category = "Design", meta = (CompactNodeTitle = "Image"))
	UImage* GetImage();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, BlueprintPure, Category = "Design", meta = (CompactNodeTitle = "Loading Throbber"))
	UCircularThrobber* GetLoadingThrobber();

	//Get the data of the item this widget is representing.
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters", meta = (CompactNodeTitle = "ItemData"))
	FS_InventoryItem GetItemData();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters", meta = (CompactNodeTitle = "ParentTile"))
	void GetParentTile(UW_Tile*& ParentTile);

	/**Get the tile widgets this item is inside.*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters", meta = (CompactNodeTitle = "OwningTiles"))
	TArray<UW_Tile*> GetOwningTiles();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters", meta = (CompactNodeTitle = "Inventory"))
	void GetInventory(UAC_Inventory*& Component);

	/**Get the widget of the container this item is inside of*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters", meta = (CompactNodeTitle = "Container"))
	UW_Container* GetParentContainer();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Destruction")
	void DestroyWidget();

	/**All data should be valid before calling this. This will update the sizing,
	 * rotation, name, etc.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "UI")
	void ConstructItemWidget();

	/**Used for custom shaped items. When dragging and rotating, you might want the item to rotate and
	 * be offset by a specific tile. In WBP_DemoInventoryItem, the anchor point is whatever tile
	 * you clicked on the item. For other types of items, you will want to return 0, 0.*/
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, BlueprintPure, Category = "UI")
	const TMap<TEnumAsByte<ERotation> ,FIntPoint> GetAnchorPoint();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "UI")
	void SetAnchorPoint(const TMap<TEnumAsByte<ERotation> ,FIntPoint>& NewAnchorPoint);

	/**Get the X and Y size of this widget, taking into account its container type
	 * and rotation.
	 * @TileSizeOverride if anything else but 0, it'll ignore the @TileSize from the parent
	 * container and use this value instead.*/
	UFUNCTION(Category = "UI", BlueprintCallable, BlueprintPure)
	FVector2D GetItemWidgetSize(FVector2D TileSizeOverride, bool IgnoreRotation = false);

	/**Set the size of the widget.
	 * Some children might be using SizeBoxes, images, or something else.
	 * This is meant to be overriden by children and modify whatever
	 * is controlling the sizing of this tile.*/
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Design")
	void SetWidgetSize(FVector2D NewSize);
	
	/**The SetPadding function has an issue where it applies padding in such
	 * a way that applies the padding to the entire widget. This causes a lot
	 * of math to break in regard to resolving this widgets size.
	 *
	 * If you wish to use padding on your widgets, I recommend either investigating
	 * this odd type of padding, or wrapping your tile image with another widget,
	 * such as an overlay, and then applying the padding to the image.
	 *
	 * This has no default implementation. You must handle this padding. */
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Design")
	void SetWidgetPadding(FMargin NewPadding);

	/**The items highlight status has been updated.
	 * By default, this will slightly scale the item
	 * to be slightly larger, then resetting the size.
	 *
	 * This is primarily used for input navigation,
	 * such as a controller gamepad*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Design")
	void ItemHighlightUpdated(bool NewIsHighlighted);

	UFUNCTION(Category = "Drag and Drop", BlueprintNativeEvent, BlueprintCallable)
	UDragDropOperation* StartDragItem();
	
	UFUNCTION(Category = "Drag and Drop", BlueprintNativeEvent, BlueprintCallable)
	void StopDragItem();

	UFUNCTION(Category = "Drag and Drop", BlueprintNativeEvent, BlueprintCallable)
	void CancelDragItem();

	//--------------------
	//Start of I_ExternalObjects interface

	virtual void BackgroundColorUpdated_Implementation(FS_InventoryItem Item, FLinearColor NewColor, bool IsTemporary) override;

	virtual FLinearColor GetBackgroundColor_Implementation() override;

	//End of I_ExternalObjects interface
	

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	
	//--------------------
	// Networking

	UFUNCTION(BlueprintImplementableEvent, Category = "Networking")
	void ParentItemAddedToNetworkQueue();

	UFUNCTION(BlueprintImplementableEvent, Category = "Networking")
	void ParentItemRemovedFromNetworkQueue();
};
