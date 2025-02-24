// Copyright (C) Varian Daemon 2023. All Rights Reserved.


#include "Core/Widgets/W_Tile.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Core/Data/FL_InventoryFramework.h"
#include "Core/Fragments/CF_TileTags.h"
#include "Core/Fragments/FL_IFP_FragmentHelpers.h"
#include "Core/Widgets/W_Container.h"
#include "Core/Widgets/W_Drag.h"
#include "Framework/Application/SlateApplication.h"


UAC_Inventory* UW_Tile::GetInventory()
{
	if(ParentContainer)
	{
		return ParentContainer->GetInventory();
	}
	else
	{
		return nullptr;
	}
}

UW_InventoryItem* UW_Tile::GetOwningItem()
{
	FS_InventoryItem FoundItem = GetInventory()->GetItemAtSpecificIndex(ParentContainer->GetContainerSettings(), TileIndex);
	if(FoundItem.IsValid())
	{
		UW_InventoryItem* ItemWidget = UFL_InventoryFramework::GetWidgetForItem(FoundItem);
		if(IsValid(ItemWidget))
		{
			return ItemWidget;
		}
	}

	return nullptr;
}

void UW_Tile::SetWidgetSize_Implementation(FVector2D NewSize)
{
	DesiredTileSize = NewSize;
}

FGameplayTagContainer UW_Tile::GetTileTags()
{
	if(IsValid(ParentContainer))
	{
		FS_ContainerSettings ContainerSettings = ParentContainer->GetContainerSettings();
		return UCF_TileTags::GetTilesTags(ContainerSettings, TileIndex);
	}

	return FGameplayTagContainer();
}

int32 UW_Tile::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                           FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
                           bool bParentEnabled) const
{
	if(SupportsHighlight)
	{
		/**Draw a box if the tile is highlighted.
		 * We continue drawing it if the opacity is 0
		 * so it smoothly transitions out when we
		 * are no longer highlighting this tile*/
		if(IsTileHighlighted || HighlightOpacity > 0)
		{
			UW_Tile* NonConstTile = const_cast<UW_Tile*>(this);
			NonConstTile->HighlightOpacity = FMath::FInterpTo(HighlightOpacity, IsTileHighlighted ? 1 : 0, FApp::GetDeltaTime(), 15);
			FPaintContext Context(AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
			UWidgetBlueprintLibrary::DrawBox(Context, FVector2D(0), DesiredTileSize, HighlightBrush, FLinearColor(1, 1, 1, HighlightOpacity));
		}
	}
	
	return Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle,
	                          bParentEnabled);
}

void UW_Tile::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	
	UAC_Inventory* LocalInventory = UFL_InventoryFramework::GetLocalInventoryComponent(this);
	if(!LocalInventory)
	{
		return;
	}

	/**If we are using IFP's custom drag drop system, then call regular OnDragEnter
	 * if we are dragging an item.*/
	if(LocalInventory->DragWidget && !LocalInventory->UseDefaultDragDropBehavior)
	{
		TSharedPtr<FDragDropOperation> SlateDragOp = FSlateApplication::Get().GetDragDroppingContent();
		FDragDropEvent DragDropEvent(InMouseEvent, SlateDragOp);
		NativeOnDragEnter(InGeometry, DragDropEvent, LocalInventory->DragWidget->DragDropOperation);
	}
}

void UW_Tile::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	/**If we are using IFP's custom drag drop system, then call regular OnDragLeave
	* if we are dragging an item.*/
	UAC_Inventory* LocalInventory = UFL_InventoryFramework::GetLocalInventoryComponent(this);
	if(!LocalInventory)
	{
		return;
	}

	if(LocalInventory->DragWidget && !LocalInventory->UseDefaultDragDropBehavior)
	{
		TSharedPtr<FDragDropOperation> SlateDragOp = FSlateApplication::Get().GetDragDroppingContent();
		FDragDropEvent DragDropEvent(InMouseEvent, SlateDragOp);
		NativeOnDragLeave(DragDropEvent, LocalInventory->DragWidget->DragDropOperation);
	}
}

