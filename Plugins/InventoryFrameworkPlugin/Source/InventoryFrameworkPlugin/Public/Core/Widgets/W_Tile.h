// Copyright (C) Varian Daemon 2023. All Rights Reserved.

// Docs: https://inventoryframework.github.io/classes-and-settings/w_tile/


#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Data/IFP_CoreData.h"
#include "W_Tile.generated.h"


class USlateBrushAsset;
class USizeBox;
class UImage;
class UW_HighlightWidget;
class UW_Drag;
class UW_InventoryItem;
struct FS_InventoryItem;
class UAC_Inventory;
class UW_Container;


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDragEnter, UW_Tile*, Tile);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDragEnd, UW_Tile*, Tile);

/**Simple widget representing the tiles inside a container widget.*/
UCLASS(Abstract)
class INVENTORYFRAMEWORKPLUGIN_API UW_Tile : public UUserWidget
{
	GENERATED_BODY()

public:

	//--------------------
	// Variables
	
	//Not exposed as the container sets this value. Can be changed in case the designer needs it changed during runtime.
	UPROPERTY(BlueprintReadWrite, Category="Settings")
	int32 TileIndex;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Parents", meta = (ExposeOnSpawn = true))
	UW_Container* ParentContainer = nullptr;

	/**Some tile children have been optimized to nothing but a simple image.
	 * But the SetDesiredSizeOverride seems to only work if the widget
	 * is on screen.
	 * This is the cached size, then when Construct is called, we call
	 * SetDesiredSizeOverride to get around this issue.*/
	UPROPERTY(BlueprintReadWrite, Category="Settings")
	FVector2D DesiredTileSize;

	UPROPERTY(BlueprintReadWrite, Category="Settings")
	bool IsTileHighlighted = false;

	/**Does this tile support being highlighted during
	 * gamepad navigation?*/
	UPROPERTY(EditDefaultsOnly, Category="Settings")
	bool SupportsHighlight = true;

	UPROPERTY(EditDefaultsOnly, Category="Settings", meta = (EditCondition = "SupportsHighlight"))
	USlateBrushAsset* HighlightBrush = nullptr;

	UPROPERTY()
	float HighlightOpacity = 0;

	
	//--------------------
	// Delegates
	
	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FDragEnter DragEnter;

	UPROPERTY(BlueprintAssignable, BlueprintCallable, Category = "EventDispatchers")
	FDragEnd DragEnd;

	
	//--------------------
	// Functions

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, BlueprintPure, Category = "Design", meta = (CompactNodeTitle = "Size Box"))
	USizeBox* GetSizeBox();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, BlueprintPure, Category = "Design", meta = (CompactNodeTitle = "Image"))
	UImage* GetImage();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters", meta = (CompactNodeTitle = "Inventory"))
	UAC_Inventory* GetInventory();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Getters", meta = (CompactNodeTitle = "Item"))
	UW_InventoryItem* GetOwningItem();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Drag")
	void UpdateDrag(UDragDropOperation* DragDropOperation);

	//V: Is this being used?
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Design")
	void SetHighlightState(bool IsHighlighted, FLinearColor Color);

	/**Set the size of the widget.
	 * Some children might be using SizeBoxes, images, or something else.
	 * This is meant to be overriden by children and modify whatever
	 * is controlling the sizing of this tile.*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Design")
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
	
	/**Can be sent by a container or other sources to customize
	 * the appearance of a specific tile.
	 * This is most often used for equipment containers where you
	 * don't want to have a custom tile class, but want a new
	 * texture background.*/
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Design")
	void ReceiveCustomBrushSettings(FSlateBrush CustomBrush);

	/**@param TargetComponent What component will handle the drop event and
	 * where we retrieve keybindings from. This should always be the
	 * player controllers controlling pawn's actor component.
	 * If you are in the editor (for example an editor utility widget)
	 * you will have to set this manually as the player controller
	 * does not exist.*/
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Item")
	void PerformDrop(UW_Drag* DragWidget, UAC_Inventory* TargetComponent);

	UFUNCTION(BlueprintCallable, Category = "Visibility", BlueprintPure, meta = (CompactNodeTitle = "Tags"))
	FGameplayTagContainer GetTileTags();
	
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
};